// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not 
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Layout/Layered/MounteaLayeredLayout.h"

#include "Algo/Reverse.h"
#include "Algo/Sort.h"

using namespace MounteaLayout;

namespace
{
	constexpr double Eps = 1.0e-6;
	/** Minimum free distance between two horizontals sharing a routing track. */
	constexpr double TrackSep = 8.0;
	/** Node-to-node-routing-lane spacing inside the BK compaction. */
	constexpr double DummyGap = 27.0;
	/** Distance of the first self-loop lane from the node edge. */
	constexpr double SelfLoopOffset = 18.0;
	/** Margin kept free at both ends of a node top when connections are combed inwards. */
	constexpr double CombMargin = 12.0;
	/** Vertical distance between staggered comb levels. */
	constexpr double JogSpacing = 6.0;
	/** Smallest horizontal clearance the router can work with, whatever the setting says. */
	constexpr double MinHorizontalGap = 24.0;

	enum class EEdgeKind : uint8 { Invalid, Forward, Feedback, SelfLoop };

	struct FLink
	{
		int32 To = INDEX_NONE;
		int32 Edge = INDEX_NONE;
		int32 Rank = 0;
	};

	struct FSegment
	{
		int32 LocalEdge = INDEX_NONE;
		double A = 0.0;
		double B = 0.0;
		int32 Channel = 0;
		int32 Source = INDEX_NONE; // local vertex whose stem this segment starts on, if any
		double Y = 0.0;
	};

	struct FComponent
	{
		TArray<int32> Nodes; // snapshot node ids, ascending
		TArray<int32> Edges; // snapshot edge indices (valid kinds), ascending

		// Working graph. Vertices [0, Nodes.Num()) are the real nodes, the rest are routing vertices.
		TArray<int32> Layer;
		TArray<double> Width;
		TArray<uint8> bDummy;
		TArray<TArray<FLink>> Succ;
		TArray<TArray<FLink>> Pred;
		TArray<TArray<int32>> Layers;
		TArray<int32> Pos;
		TMap<int32, TArray<int32>> Chain; // snapshot edge -> vertex chain
		int32 LayerCount = 0;

		int64 Crossings = 0;
		int32 StallCount = 0;
		bool bConverged = false;
		TArray<TArray<int32>> BestLayers;

		// Geometry, component-local.
		TArray<FIntPoint> NodePos;           // parallel to Nodes
		TArray<FDialogueLayoutRoute> Routes; // parallel to Edges
		double MinX = 0.0;
		double MaxX = 0.0;
	};

	int64 CountInversions(TArray<TPair<int32, int32>>& Pairs, int32 LowerCount)
	{
		Pairs.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B)
		{
			return A.Key != B.Key ? A.Key < B.Key : A.Value < B.Value;
		});

		TArray<int32> bit;
		bit.Init(0, LowerCount + 2);
		int64 crossings = 0;
		int32 inserted = 0;
		for (const TPair<int32, int32>& pair : Pairs)
		{
			int32 notGreater = 0;
			for (int32 i = pair.Value + 1; i > 0; i -= i & -i)
				notGreater += bit[i];
			crossings += inserted - notGreater;
			for (int32 i = pair.Value + 1; i <= LowerCount; i += i & -i)
				++bit[i];
			++inserted;
		}
		return crossings;
	}

	struct FRect
	{
		double L = 0, T = 0, R = 0, B = 0;
		int32 Owner = INDEX_NONE; // node id, or edge index for controls
		bool bControl = false;
	};

	bool ClipSegmentToRect(FVector2D A, FVector2D B, const FRect& Rect)
	{
		double t0 = 0.0, t1 = 1.0;
		const FVector2D d = B - A;
		const auto clip = [&](double P, double Q) -> bool
		{
			if (FMath::Abs(P) < Eps)
				return Q >= 0.0;
			const double r = Q / P;
			if (P < 0.0)
			{
				if (r > t1) return false;
				t0 = FMath::Max(t0, r);
			}
			else
			{
				if (r < t0) return false;
				t1 = FMath::Min(t1, r);
			}
			return true;
		};
		return clip(-d.X, A.X - Rect.L) && clip(d.X, Rect.R - A.X) && clip(-d.Y, A.Y - Rect.T) && clip(d.Y, Rect.B - A.Y);
	}

	/** Coarse uniform grid over rectangles. */
	class FRectGrid
	{
	public:
		explicit FRectGrid(const TArray<FRect>& InRects) : Rects(InRects)
		{
			for (int32 i = 0; i < Rects.Num(); ++i)
			{
				int32 x0, y0, x1, y1;
				CellRange(Rects[i].L, Rects[i].T, Rects[i].R, Rects[i].B, x0, y0, x1, y1);
				for (int32 y = y0; y <= y1; ++y)
					for (int32 x = x0; x <= x1; ++x)
						Cells.FindOrAdd(Key(x, y)).Add(i);
			}
		}

		void Query(double L, double T, double R, double B, TArray<int32>& Out) const
		{
			Out.Reset();
			int32 x0, y0, x1, y1;
			CellRange(L, T, R, B, x0, y0, x1, y1);
			if ((int64)(x1 - x0 + 1) * (y1 - y0 + 1) > 4096)
			{
				for (int32 i = 0; i < Rects.Num(); ++i)
					Out.Add(i);
				return;
			}
			for (int32 y = y0; y <= y1; ++y)
				for (int32 x = x0; x <= x1; ++x)
					if (const TArray<int32>* cell = Cells.Find(Key(x, y)))
						for (int32 index : *cell)
							Out.AddUnique(index);
		}

	private:
		static constexpr double CellSize = 256.0;
		static int64 Key(int32 X, int32 Y) { return (static_cast<int64>(X) << 32) ^ static_cast<uint32>(Y); }
		static int32 Cell(double V) { return static_cast<int32>(FMath::Clamp(FMath::FloorToDouble(V / CellSize), -1.0e8, 1.0e8)); }
		static void CellRange(double L, double T, double R, double B, int32& X0, int32& Y0, int32& X1, int32& Y1)
		{
			X0 = Cell(FMath::Min(L, R)); X1 = Cell(FMath::Max(L, R));
			Y0 = Cell(FMath::Min(T, B)); Y1 = Cell(FMath::Max(T, B));
		}

		const TArray<FRect>& Rects;
		TMap<int64, TArray<int32>> Cells;
	};

	// ------------------------------------------------------------------------------------------------
	// Brandes-Koepf horizontal coordinate assignment (one of four alignments).
	// ------------------------------------------------------------------------------------------------
	struct FBKVariant
	{
		const FComponent& C;
		const bool bDown;
		const bool bLeft;

		TArray<TArray<int32>> Lay;
		TArray<int32> TL; // transformed layer per vertex
		TArray<int32> P;  // position in transformed layer
		TArray<TArray<int32>> Up;
		TArray<int32> Root, Align, Sink;
		TArray<double> Shift, X;
		TArray<uint8> Placed;
		TSet<uint64> Marked;

		FBKVariant(const FComponent& InC, bool bInDown, bool bInLeft) : C(InC), bDown(bInDown), bLeft(bInLeft) {}

		double Delta(int32 A, int32 B) const
		{
			const bool dummyA = C.bDummy[A] != 0;
			const bool dummyB = C.bDummy[B] != 0;
			double gap = DummyGap;
			if (!dummyA && !dummyB)
				gap = HorizontalGap;
			else if (dummyA != dummyB)
				gap = LaneClearance;
			return (C.Width[A] + C.Width[B]) * 0.5 + gap;
		}

		static uint64 EdgeKey(int32 U, int32 V) { return (static_cast<uint64>(static_cast<uint32>(U)) << 32) | static_cast<uint32>(V); }

		void PlaceBlock(int32 V)
		{
			if (Placed[V])
				return;
			Placed[V] = 1;
			X[V] = 0.0;
			int32 w = V;
			do
			{
				if (P[w] > 0)
				{
					const int32 predecessor = Lay[TL[w]][P[w] - 1];
					const int32 u = Root[predecessor];
					PlaceBlock(u);
					if (Sink[V] == V)
						Sink[V] = Sink[u];
					if (Sink[V] != Sink[u])
						Shift[Sink[u]] = FMath::Min(Shift[Sink[u]], X[V] - X[u] - Delta(predecessor, w));
					else
						X[V] = FMath::Max(X[V], X[u] + Delta(predecessor, w));
				}
				w = Align[w];
			} while (w != V);
		}

		void Run(TArray<double>& OutX, double InHorizontalGap)
		{
			HorizontalGap = InHorizontalGap;
			const int32 vertexCount = C.Layer.Num();
			const int32 layerCount = C.LayerCount;

			Lay.SetNum(layerCount);
			TL.SetNum(vertexCount);
			P.SetNum(vertexCount);
			Up.SetNum(vertexCount);
			for (int32 l = 0; l < layerCount; ++l)
			{
				const int32 source = bDown ? l : layerCount - 1 - l;
				Lay[l] = C.Layers[source];
				if (!bLeft)
					Algo::Reverse(Lay[l]);
				for (int32 i = 0; i < Lay[l].Num(); ++i)
				{
					TL[Lay[l][i]] = l;
					P[Lay[l][i]] = i;
				}
			}
			for (int32 v = 0; v < vertexCount; ++v)
			{
				for (const FLink& link : bDown ? C.Pred[v] : C.Succ[v])
					Up[v].AddUnique(link.To);
				Up[v].Sort([this](int32 A, int32 B) { return P[A] < P[B]; });
			}

			MarkConflicts();
			AlignVertically();
			Compact(vertexCount);

			OutX.SetNum(vertexCount);
			for (int32 v = 0; v < vertexCount; ++v)
			{
				double x = X[Root[v]];
				const double shift = Shift[Sink[Root[v]]];
				if (shift < TNumericLimits<double>::Max())
					x += shift;
				OutX[v] = bLeft ? x : -x;
			}
		}

	private:
		double HorizontalGap = 0.0;

		void MarkConflicts()
		{
			const int32 h = Lay.Num();
			for (int32 i = 1; i + 2 < h; ++i)
			{
				const TArray<int32>& upper = Lay[i];
				const TArray<int32>& lower = Lay[i + 1];
				int32 k0 = 0;
				int32 l = 0;
				for (int32 l1 = 0; l1 < lower.Num(); ++l1)
				{
					const int32 v = lower[l1];
					int32 innerPos = INDEX_NONE;
					if (C.bDummy[v])
					{
						for (int32 u : Up[v])
							if (C.bDummy[u])
								innerPos = P[u];
					}
					if (l1 == lower.Num() - 1 || innerPos != INDEX_NONE)
					{
						const int32 k1 = innerPos != INDEX_NONE ? innerPos : upper.Num() - 1;
						for (; l <= l1; ++l)
						{
							for (int32 u : Up[lower[l]])
							{
								const int32 k = P[u];
								if (k < k0 || k > k1)
									Marked.Add(EdgeKey(u, lower[l]));
							}
						}
						k0 = k1;
					}
				}
			}
		}

		void AlignVertically()
		{
			const int32 vertexCount = C.Layer.Num();
			Root.SetNum(vertexCount);
			Align.SetNum(vertexCount);
			for (int32 v = 0; v < vertexCount; ++v)
			{
				Root[v] = v;
				Align[v] = v;
			}

			for (const TArray<int32>& layer : Lay)
			{
				int32 r = -1;
				for (int32 v : layer)
				{
					const TArray<int32>& up = Up[v];
					if (up.Num() == 0)
						continue;
					const int32 lo = (up.Num() - 1) / 2;
					const int32 hi = up.Num() / 2;
					for (int32 m = lo; m <= hi; ++m)
					{
						if (Align[v] != v)
							break;
						const int32 u = up[m];
						if (!Marked.Contains(EdgeKey(u, v)) && r < P[u])
						{
							Align[u] = v;
							Root[v] = Root[u];
							Align[v] = Root[v];
							r = P[u];
						}
					}
				}
			}
		}

		void Compact(int32 VertexCount)
		{
			Sink.SetNum(VertexCount);
			Shift.Init(TNumericLimits<double>::Max(), VertexCount);
			X.Init(0.0, VertexCount);
			Placed.Init(0, VertexCount);
			for (int32 v = 0; v < VertexCount; ++v)
				Sink[v] = v;
			for (int32 v = 0; v < VertexCount; ++v)
				if (Root[v] == v)
					PlaceBlock(v);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Engine
	// ------------------------------------------------------------------------------------------------
	class FEngine
	{
	public:
		FEngine(const FDialogueLayoutSnapshot& InSnapshot, FDialogueLayoutTaskState& InState)
			: S(InSnapshot), State(InState) {}

		bool Execute();

	private:
		const FDialogueLayoutSnapshot& S;
		FDialogueLayoutTaskState& State;

		TArray<FVector2D> NodeSize;
		TArray<EEdgeKind> Kind;
		TArray<int32> EdgeRank;
		TArray<int32> IncomingCount;
		TArray<TArray<int32>> OutEdges;
		TArray<FComponent> Comps;
		TArray<int32> NodeComp;
		TArray<int32> NodeLocal;
		int32 NextSequence = 1;
		int64 PublishedCrossings = TNumericLimits<int64>::Max();

		bool Fail(const FString& Message)
		{
			State.AddDiagnostic(Message);
			State.SetProgress(EDialogueLayoutPhase::Failed, 0, 0);
			return false;
		}
		bool Cancelled() const { return State.IsCancelRequested(); }

		bool Prepare();
		bool BuildComponent(FComponent& C);
		void InitialOrder(FComponent& C);
		int64 CountCrossings(const FComponent& C) const;
		void UpdatePositions(FComponent& C, int32 Layer) const;
		void SweepOnce(FComponent& C, bool bDown);
		void RepairPortOrder(FComponent& C, int32 Layer) const;
		void ImproveBySwaps(FComponent& C, int32 Layer) const;
		void ComputeXCenters(const FComponent& C, TArray<double>& OutX) const;
		bool BuildGeometry(FComponent& C);
		bool Assemble(FDialogueLayoutCheckpoint& Out) const;
		bool PublishIfValid(bool bRequireImprovement);
	};

	bool FEngine::Prepare()
	{
		State.SetProgress(EDialogueLayoutPhase::Classifying, 0, 1);

		const int32 nodeCount = S.Nodes.Num();
		for (int32 i = 0; i < nodeCount; ++i)
		{
			if (S.Nodes[i].Id != i)
				return Fail(FString::Printf(TEXT("Layout input invalid: node at index %d has id %d (ids must be dense and unique)."), i, S.Nodes[i].Id));
		}

		if (S.AnchorId < 0 || S.AnchorId >= nodeCount || !S.Nodes[S.AnchorId].bIsStart)
			return Fail(TEXT("Layout input invalid: the Start node could not be resolved, so no arrangement was calculated."));

		int32 unmeasured = 0;
		NodeSize.SetNum(nodeCount);
		for (int32 i = 0; i < nodeCount; ++i)
		{
			FVector2D size = S.Nodes[i].Size;
			if (!FMath::IsFinite(size.X) || !FMath::IsFinite(size.Y) || size.X <= 0.0 || size.Y <= 0.0)
			{
				size = FVector2D(FallbackNodeWidth, FallbackNodeHeight);
				++unmeasured;
			}
			NodeSize[i] = size;
		}
		if (unmeasured > 0)
			State.AddDiagnostic(FString::Printf(TEXT("%d node(s) had no usable size; the fallback size was used."), unmeasured));

		const int32 edgeCount = S.Edges.Num();
		Kind.Init(EEdgeKind::Invalid, edgeCount);
		OutEdges.SetNum(nodeCount);
		IncomingCount.Init(0, nodeCount);
		TArray<int32> forwardInDegree;
		forwardInDegree.Init(0, nodeCount);
		int32 invalid = 0;
		for (int32 e = 0; e < edgeCount; ++e)
		{
			const FDialogueLayoutEdge& edge = S.Edges[e];
			if (edge.From < 0 || edge.From >= nodeCount || edge.To < 0 || edge.To >= nodeCount)
			{
				++invalid;
				continue;
			}
			Kind[e] = EEdgeKind::Forward; // provisional: valid
			OutEdges[edge.From].Add(e);
			++IncomingCount[edge.To];
			if (edge.From != edge.To)
				++forwardInDegree[edge.To];
		}
		if (invalid > 0)
			State.AddDiagnostic(FString::Printf(TEXT("%d connection(s) reference missing nodes and were ignored."), invalid));

		EdgeRank.Init(0, edgeCount);
		for (int32 n = 0; n < nodeCount; ++n)
		{
			OutEdges[n].Sort([this](int32 A, int32 B)
			{
				const auto key = [this](int32 E)
				{
					const int32 order = S.Nodes[S.Edges[E].To].ExecutionOrder;
					return order < 0 ? TNumericLimits<int32>::Max() : order;
				};
				const int32 ka = key(A), kb = key(B);
				if (ka != kb) return ka < kb;
				if (S.Edges[A].To != S.Edges[B].To) return S.Edges[A].To < S.Edges[B].To;
				return A < B;
			});
			for (int32 i = 0; i < OutEdges[n].Num(); ++i)
				EdgeRank[OutEdges[n][i]] = i;
		}

		// Iterative DFS: back edges become feedback edges in the calculation copy only.
		TArray<int32> roots;
		roots.Add(S.AnchorId);
		for (int32 n = 0; n < nodeCount; ++n)
			if (n != S.AnchorId && forwardInDegree[n] == 0)
				roots.Add(n);
		for (int32 n = 0; n < nodeCount; ++n)
			if (n != S.AnchorId && forwardInDegree[n] != 0)
				roots.Add(n);

		TArray<uint8> color;
		color.Init(0, nodeCount);
		struct FFrame { int32 Node; int32 Next; };
		TArray<FFrame> stack;
		for (int32 root : roots)
		{
			if (color[root] != 0)
				continue;
			color[root] = 1;
			stack.Add({root, 0});
			while (stack.Num() > 0)
			{
				if (Cancelled())
					return true;
				const int32 top = stack.Num() - 1;
				const int32 u = stack[top].Node;
				if (stack[top].Next < OutEdges[u].Num())
				{
					const int32 e = OutEdges[u][stack[top].Next++];
					const int32 v = S.Edges[e].To;
					if (v == u)
						Kind[e] = EEdgeKind::SelfLoop;
					else if (color[v] == 1)
						Kind[e] = EEdgeKind::Feedback;
					else
					{
						Kind[e] = EEdgeKind::Forward;
						if (color[v] == 0)
						{
							color[v] = 1;
							stack.Add({v, 0});
						}
					}
				}
				else
				{
					color[u] = 2;
					stack.Pop();
				}
			}
		}

		// Connected components (union-find over every valid connection).
		TArray<int32> parent;
		parent.SetNum(nodeCount);
		for (int32 i = 0; i < nodeCount; ++i)
			parent[i] = i;
		const auto find = [&parent](int32 X)
		{
			while (parent[X] != X)
			{
				parent[X] = parent[parent[X]];
				X = parent[X];
			}
			return X;
		};
		for (int32 e = 0; e < edgeCount; ++e)
		{
			if (Kind[e] == EEdgeKind::Invalid)
				continue;
			const int32 a = find(S.Edges[e].From);
			const int32 b = find(S.Edges[e].To);
			if (a != b)
				parent[FMath::Max(a, b)] = FMath::Min(a, b);
		}

		TMap<int32, int32> compOfRoot;
		NodeComp.SetNum(nodeCount);
		NodeLocal.SetNum(nodeCount);
		const int32 anchorRoot = find(S.AnchorId);
		compOfRoot.Add(anchorRoot, 0);
		Comps.AddDefaulted();
		for (int32 n = 0; n < nodeCount; ++n)
		{
			const int32 root = find(n);
			int32* index = compOfRoot.Find(root);
			if (!index)
			{
				index = &compOfRoot.Add(root, Comps.Num());
				Comps.AddDefaulted();
			}
			NodeComp[n] = *index;
			NodeLocal[n] = Comps[*index].Nodes.Num();
			Comps[*index].Nodes.Add(n);
		}
		for (int32 e = 0; e < edgeCount; ++e)
			if (Kind[e] != EEdgeKind::Invalid)
				Comps[NodeComp[S.Edges[e].From]].Edges.Add(e);

		return true;
	}

	bool FEngine::BuildComponent(FComponent& C)
	{
		const int32 realCount = C.Nodes.Num();
		C.Layer.Init(0, realCount);

		TArray<TArray<int32>> forward;
		forward.SetNum(realCount);
		TArray<int32> inDegree;
		inDegree.Init(0, realCount);
		for (int32 e : C.Edges)
		{
			if (Kind[e] != EEdgeKind::Forward)
				continue;
			forward[NodeLocal[S.Edges[e].From]].Add(e);
			++inDegree[NodeLocal[S.Edges[e].To]];
		}

		TArray<int32> queue;
		for (int32 v = 0; v < realCount; ++v)
			if (inDegree[v] == 0)
				queue.Add(v);
		for (int32 head = 0; head < queue.Num(); ++head)
		{
			const int32 u = queue[head];
			for (int32 e : forward[u])
			{
				const int32 v = NodeLocal[S.Edges[e].To];
				C.Layer[v] = FMath::Max(C.Layer[v], C.Layer[u] + 1);
				if (--inDegree[v] == 0)
					queue.Add(v);
			}
		}
		if (queue.Num() != realCount)
			return Fail(TEXT("Internal layout error: the working graph is not acyclic after feedback classification."));

		int64 dummyCount = 0;
		for (int32 e : C.Edges)
			if (Kind[e] == EEdgeKind::Forward)
				dummyCount += C.Layer[NodeLocal[S.Edges[e].To]] - C.Layer[NodeLocal[S.Edges[e].From]] - 1;
		if (realCount + dummyCount > S.Limits.MaxRoutingVertices)
			return Fail(FString::Printf(TEXT("Layout budget exhausted: %lld routing vertices needed, limit is %d."), static_cast<long long>(realCount + dummyCount), S.Limits.MaxRoutingVertices));

		C.Width.SetNum(realCount);
		C.bDummy.Init(0, realCount);
		C.Succ.SetNum(realCount);
		C.Pred.SetNum(realCount);
		TArray<int32> selfLoops;
		selfLoops.Init(0, realCount);
		for (int32 e : C.Edges)
			if (Kind[e] == EEdgeKind::SelfLoop)
				++selfLoops[NodeLocal[S.Edges[e].From]];
		for (int32 v = 0; v < realCount; ++v)
		{
			// Room for control slots and, if present, self-loop lanes on both sides (keeps the node centred).
			const double lanes = selfLoops[v] > 0 ? 2.0 * (SelfLoopOffset + selfLoops[v] * LaneSpacing) : 0.0;
			C.Width[v] = FMath::Max(NodeSize[C.Nodes[v]].X, IncomingCount[C.Nodes[v]] * BubblePitch) + lanes;
		}

		for (int32 e : C.Edges)
		{
			if (Kind[e] != EEdgeKind::Forward)
				continue;
			const int32 u = NodeLocal[S.Edges[e].From];
			const int32 v = NodeLocal[S.Edges[e].To];
			TArray<int32>& chain = C.Chain.Add(e);
			chain.Add(u);
			for (int32 l = C.Layer[u] + 1; l < C.Layer[v]; ++l)
			{
				const int32 dummy = C.Layer.Add(l);
				C.Width.Add(0.0);
				C.bDummy.Add(1);
				C.Succ.AddDefaulted();
				C.Pred.AddDefaulted();
				chain.Add(dummy);
			}
			chain.Add(v);
			for (int32 i = 0; i + 1 < chain.Num(); ++i)
			{
				C.Succ[chain[i]].Add({chain[i + 1], e, EdgeRank[e]});
				C.Pred[chain[i + 1]].Add({chain[i], e, EdgeRank[e]});
			}
		}

		const auto linkLess = [](const FLink& A, const FLink& B)
		{
			return A.Rank != B.Rank ? A.Rank < B.Rank : A.Edge < B.Edge;
		};
		for (TArray<FLink>& links : C.Succ)
			links.Sort(linkLess);
		for (TArray<FLink>& links : C.Pred)
			links.Sort(linkLess);

		int32 maxLayer = 0;
		for (int32 layer : C.Layer)
			maxLayer = FMath::Max(maxLayer, layer);
		C.LayerCount = maxLayer + 1;
		InitialOrder(C);

		C.Crossings = CountCrossings(C);
		C.BestLayers = C.Layers;
		C.bConverged = C.Crossings == 0;
		return true;
	}

	void FEngine::InitialOrder(FComponent& C)
	{
		const int32 vertexCount = C.Layer.Num();
		C.Layers.Reset();
		C.Layers.SetNum(C.LayerCount);
		C.Pos.Init(0, vertexCount);

		TArray<int32> sources;
		const int32 anchorLocal = NodeComp[S.AnchorId] < Comps.Num() && &Comps[NodeComp[S.AnchorId]] == &C ? NodeLocal[S.AnchorId] : INDEX_NONE;
		if (anchorLocal != INDEX_NONE && C.Pred[anchorLocal].Num() == 0)
			sources.Add(anchorLocal);
		for (int32 v = 0; v < C.Nodes.Num(); ++v)
			if (v != anchorLocal && C.Pred[v].Num() == 0)
				sources.Add(v);

		TArray<uint8> visited;
		visited.Init(0, vertexCount);
		struct FFrame { int32 V; int32 Next; };
		TArray<FFrame> stack;
		for (int32 source : sources)
		{
			if (visited[source])
				continue;
			visited[source] = 1;
			C.Layers[C.Layer[source]].Add(source);
			stack.Add({source, 0});
			while (stack.Num() > 0)
			{
				const int32 top = stack.Num() - 1;
				const int32 v = stack[top].V;
				if (stack[top].Next < C.Succ[v].Num())
				{
					const int32 to = C.Succ[v][stack[top].Next++].To;
					if (!visited[to])
					{
						visited[to] = 1;
						C.Layers[C.Layer[to]].Add(to);
						stack.Add({to, 0});
					}
				}
				else
					stack.Pop();
			}
		}
		for (int32 l = 0; l < C.LayerCount; ++l)
			UpdatePositions(C, l);
	}

	void FEngine::UpdatePositions(FComponent& C, int32 Layer) const
	{
		for (int32 i = 0; i < C.Layers[Layer].Num(); ++i)
			C.Pos[C.Layers[Layer][i]] = i;
	}

	int64 FEngine::CountCrossings(const FComponent& C) const
	{
		int64 total = 0;
		TArray<TPair<int32, int32>> pairs;
		for (int32 l = 0; l + 1 < C.LayerCount; ++l)
		{
			pairs.Reset();
			for (int32 v : C.Layers[l])
				for (const FLink& link : C.Succ[v])
					pairs.Add({C.Pos[v], C.Pos[link.To]});
			total += CountInversions(pairs, C.Layers[l + 1].Num());
		}
		return total;
	}

	void FEngine::RepairPortOrder(FComponent& C, int32 Layer) const
	{
		if (Layer <= 0)
			return;

		TArray<TPair<int32, int32>> children; // (rank, vertex)
		TArray<int32> slots;
		for (int32 parentVertex : C.Layers[Layer - 1])
		{
			children.Reset();
			for (const FLink& link : C.Succ[parentVertex])
			{
				bool bKnown = false;
				for (const TPair<int32, int32>& entry : children)
					bKnown |= entry.Value == link.To;
				if (!bKnown)
					children.Add({link.Rank, link.To});
			}
			if (children.Num() < 2)
				continue;

			slots.Reset();
			for (const TPair<int32, int32>& entry : children)
				slots.Add(C.Pos[entry.Value]);
			slots.Sort();
			children.Sort([&C](const TPair<int32, int32>& A, const TPair<int32, int32>& B)
			{
				return A.Key != B.Key ? A.Key < B.Key : C.Pos[A.Value] < C.Pos[B.Value];
			});
			for (int32 i = 0; i < children.Num(); ++i)
				C.Layers[Layer][slots[i]] = children[i].Value;
			UpdatePositions(C, Layer);
		}
	}

	void FEngine::ImproveBySwaps(FComponent& C, int32 Layer) const
	{
		TArray<int32>& layer = C.Layers[Layer];
		const auto crossingsIfLeft = [&C](int32 A, int32 B)
		{
			int32 count = 0;
			const auto accumulate = [&](const TArray<FLink>& NbA, const TArray<FLink>& NbB)
			{
				for (const FLink& a : NbA)
					for (const FLink& b : NbB)
						if (C.Pos[a.To] > C.Pos[b.To])
							++count;
			};
			accumulate(C.Pred[A], C.Pred[B]);
			accumulate(C.Succ[A], C.Succ[B]);
			return count;
		};
		const auto swapForbidden = [&C](int32 Left, int32 Right)
		{
			for (const FLink& a : C.Pred[Left])
				for (const FLink& b : C.Pred[Right])
					if (a.To == b.To && a.Rank < b.Rank)
						return true;
			return false;
		};

		for (int32 pass = 0; pass < 4; ++pass)
		{
			bool bImproved = false;
			for (int32 i = 0; i + 1 < layer.Num(); ++i)
			{
				const int32 u = layer[i];
				const int32 v = layer[i + 1];
				if (swapForbidden(u, v))
					continue;
				if (crossingsIfLeft(v, u) < crossingsIfLeft(u, v))
				{
					layer[i] = v;
					layer[i + 1] = u;
					C.Pos[v] = i;
					C.Pos[u] = i + 1;
					bImproved = true;
				}
			}
			if (!bImproved || Cancelled())
				break;
		}
	}

	void FEngine::SweepOnce(FComponent& C, bool bDown)
	{
		const int32 first = bDown ? 1 : C.LayerCount - 2;
		const int32 last = bDown ? C.LayerCount : -1;
		const int32 step = bDown ? 1 : -1;

		TArray<TPair<double, int32>> keyed;
		TArray<int32> slots;
		TArray<int32> positions;
		for (int32 l = first; l != last; l += step)
		{
			if (Cancelled())
				return;

			TArray<int32>& layer = C.Layers[l];
			keyed.Reset();
			slots.Reset();
			for (int32 i = 0; i < layer.Num(); ++i)
			{
				const int32 v = layer[i];
				const TArray<FLink>& neighbours = bDown ? C.Pred[v] : C.Succ[v];
				if (neighbours.Num() == 0)
					continue;
				positions.Reset();
				for (const FLink& link : neighbours)
					positions.Add(C.Pos[link.To]);
				positions.Sort();
				const int32 count = positions.Num();
				const double median = (count % 2 == 1) ? positions[count / 2] : (positions[count / 2 - 1] + positions[count / 2]) * 0.5;
				keyed.Add({median, v});
				slots.Add(i);
			}
			keyed.Sort([&C](const TPair<double, int32>& A, const TPair<double, int32>& B)
			{
				return A.Key != B.Key ? A.Key < B.Key : C.Pos[A.Value] < C.Pos[B.Value];
			});
			for (int32 k = 0; k < keyed.Num(); ++k)
				layer[slots[k]] = keyed[k].Value;
			UpdatePositions(C, l);

			// Keep semantic (option) order at shared ports.
			RepairPortOrder(C, l);
			ImproveBySwaps(C, l);
			RepairPortOrder(C, l);
			UpdatePositions(C, l);
		}
	}

	void FEngine::ComputeXCenters(const FComponent& C, TArray<double>& OutX) const
	{
		const int32 vertexCount = C.Layer.Num();
		TArray<TArray<double>> variants;
		variants.SetNum(4);
		const double gap = FMath::Max(S.HorizontalClearance, MinHorizontalGap);
		for (int32 variant = 0; variant < 4; ++variant)
		{
			FBKVariant bk(C, (variant & 1) == 0, (variant & 2) == 0);
			bk.Run(variants[variant], gap);
		}

		// Align to the narrowest layout, then take the average of the two median candidates.
		double minOf[4], maxOf[4];
		int32 narrowest = 0;
		for (int32 variant = 0; variant < 4; ++variant)
		{
			minOf[variant] = TNumericLimits<double>::Max();
			maxOf[variant] = TNumericLimits<double>::Lowest();
			for (double x : variants[variant])
			{
				minOf[variant] = FMath::Min(minOf[variant], x);
				maxOf[variant] = FMath::Max(maxOf[variant], x);
			}
			if (maxOf[variant] - minOf[variant] < maxOf[narrowest] - minOf[narrowest])
				narrowest = variant;
		}
		for (int32 variant = 0; variant < 4; ++variant)
		{
			const bool bLeft = (variant & 2) == 0;
			const double shift = bLeft ? minOf[narrowest] - minOf[variant] : maxOf[narrowest] - maxOf[variant];
			for (double& x : variants[variant])
				x += shift;
		}

		OutX.SetNum(vertexCount);
		for (int32 v = 0; v < vertexCount; ++v)
		{
			double values[4] = {variants[0][v], variants[1][v], variants[2][v], variants[3][v]};
			Algo::Sort(TArrayView<double>(values, 4));
			OutX[v] = (values[1] + values[2]) * 0.5;
		}

		// Guarantee minimum separation after averaging.
		for (const TArray<int32>& layer : C.Layers)
		{
			for (int32 i = 1; i < layer.Num(); ++i)
			{
				const int32 a = layer[i - 1];
				const int32 b = layer[i];
				const double gapAB = (C.bDummy[a] && C.bDummy[b]) ? DummyGap : ((!C.bDummy[a] && !C.bDummy[b]) ? gap : LaneClearance);
				OutX[b] = FMath::Max(OutX[b], OutX[a] + (C.Width[a] + C.Width[b]) * 0.5 + gapAB);
			}
		}
	}

	bool FEngine::BuildGeometry(FComponent& C)
	{
		const int32 realCount = C.Nodes.Num();
		const int32 layerCount = C.LayerCount;

		State.SetProgress(EDialogueLayoutPhase::Coordinates, 0, 1);
		TArray<double> xc;
		ComputeXCenters(C, xc);
		if (Cancelled())
			return false;

		// Real node boxes.
		TArray<double> left, cx;
		left.SetNum(realCount);
		cx.SetNum(realCount);
		for (int32 v = 0; v < realCount; ++v)
		{
			const double w = NodeSize[C.Nodes[v]].X;
			left[v] = FMath::RoundToDouble(xc[v] - w * 0.5);
			cx[v] = left[v] + w * 0.5;
		}
		const auto centerOf = [&](int32 V) { return V < realCount ? cx[V] : xc[V]; };

		// Entry slots at targets, ordered by where the connection arrives from.
		TArray<TArray<int32>> incoming;
		incoming.SetNum(realCount);
		TArray<double> arriveFrom;
		arriveFrom.Init(0.0, S.Edges.Num());
		for (int32 e : C.Edges)
		{
			const int32 target = NodeLocal[S.Edges[e].To];
			incoming[target].Add(e);
			if (Kind[e] == EEdgeKind::Forward)
			{
				const TArray<int32>& chain = C.Chain[e];
				arriveFrom[e] = centerOf(chain[chain.Num() - 2]);
			}
			else
				arriveFrom[e] = TNumericLimits<double>::Max();
		}
		// entryX: where the connection (and its control) comes down. innerX: where it finally meets the node top.
		// With too many connections for the node width the final approach "combs" inwards in staggered jog levels.
		TArray<double> entryX, innerX;
		TArray<int32> jogLevel;
		entryX.Init(0.0, S.Edges.Num());
		innerX.Init(0.0, S.Edges.Num());
		jogLevel.Init(INDEX_NONE, S.Edges.Num());
		TArray<double> nodeJog; // vertical room above the node needed by its comb
		nodeJog.Init(0.0, realCount);
		for (int32 v = 0; v < realCount; ++v)
		{
			incoming[v].Sort([&arriveFrom](int32 A, int32 B) { return arriveFrom[A] != arriveFrom[B] ? arriveFrom[A] < arriveFrom[B] : A < B; });
			const int32 count = incoming[v].Num();
			const double nodeWidth = NodeSize[C.Nodes[v]].X;
			const bool bComb = count > 1 && (count - 1) * BubblePitch > nodeWidth - 2.0 * CombMargin;
			const double step = bComb ? (nodeWidth - 2.0 * CombMargin) / (count - 1) : 0.0;
			for (int32 i = 0; i < count; ++i)
			{
				const int32 e = incoming[v][i];
				const double offset = i - (count - 1) * 0.5;
				entryX[e] = cx[v] + offset * BubblePitch;
				innerX[e] = bComb ? cx[v] + offset * step : entryX[e];
				// Outermost connections jog lowest so no vertical crosses another connection's jog.
				jogLevel[e] = bComb ? FMath::Min(i, count - 1 - i) : INDEX_NONE;
			}
			if (bComb)
				nodeJog[v] = (FMath::DivideAndRoundUp(count, 2)) * JogSpacing;
		}

		// Exterior lanes (feedback) and side lanes (self loops).
		double rightmost = TNumericLimits<double>::Lowest();
		for (int32 v = 0; v < C.Layer.Num(); ++v)
			rightmost = FMath::Max(rightmost, centerOf(v) + C.Width[v] * 0.5);
		TArray<double> laneX;
		laneX.Init(0.0, S.Edges.Num());
		int32 feedbackIndex = 0;
		TArray<int32> selfIndex;
		selfIndex.Init(0, realCount);
		for (int32 e : C.Edges)
		{
			if (Kind[e] == EEdgeKind::Feedback)
				laneX[e] = rightmost + LaneClearance + (feedbackIndex++) * LaneSpacing;
			else if (Kind[e] == EEdgeKind::SelfLoop)
			{
				const int32 u = NodeLocal[S.Edges[e].From];
				laneX[e] = cx[u] + NodeSize[C.Nodes[u]].X * 0.5 + SelfLoopOffset + (selfIndex[u]++) * LaneSpacing;
			}
		}
		const double lanesRight = rightmost + LaneClearance + FMath::Max(feedbackIndex, 0) * LaneSpacing;

		// Horizontal segments per channel. Channel c lies below layer c; c == -1 is above layer 0.
		TArray<FSegment> segments;
		TArray<TArray<int32>> edgeSegments;
		edgeSegments.SetNum(C.Edges.Num());
		for (int32 le = 0; le < C.Edges.Num(); ++le)
		{
			const int32 e = C.Edges[le];
			const int32 u = NodeLocal[S.Edges[e].From];
			const int32 v = NodeLocal[S.Edges[e].To];
			if (Kind[e] == EEdgeKind::Forward)
			{
				const TArray<int32>& chain = C.Chain[e];
				for (int32 hop = 0; hop + 1 < chain.Num(); ++hop)
				{
					const double xa = hop == 0 ? cx[u] : xc[chain[hop]];
					const double xb = hop + 2 == chain.Num() ? entryX[e] : xc[chain[hop + 1]];
					if (FMath::Abs(xa - xb) < Eps)
					{
						edgeSegments[le].Add(INDEX_NONE);
						continue;
					}
					edgeSegments[le].Add(segments.Num());
					segments.Add({le, xa, xb, C.Layer[chain[hop]], hop == 0 ? u : INDEX_NONE, 0.0});
				}
			}
			else
			{
				edgeSegments[le].Add(segments.Num());
				segments.Add({le, cx[u], laneX[e], C.Layer[u], u, 0.0});
				edgeSegments[le].Add(segments.Num());
				segments.Add({le, laneX[e], entryX[e], C.Layer[v] - 1, INDEX_NONE, 0.0});
			}
		}

		// Track assignment (left-edge algorithm, per channel).
		const int32 channelCount = layerCount + 1;
		TArray<TArray<int32>> byChannel;
		byChannel.SetNum(channelCount);
		for (int32 s = 0; s < segments.Num(); ++s)
			byChannel[segments[s].Channel + 1].Add(s);

		TArray<int32> trackCount;
		trackCount.Init(0, channelCount);
		TArray<int32> trackOf;
		trackOf.Init(0, segments.Num());
		for (int32 channel = 0; channel < channelCount; ++channel)
		{
			TArray<int32>& list = byChannel[channel];
			list.Sort([&segments](int32 A, int32 B)
			{
				const double la = FMath::Min(segments[A].A, segments[A].B), lb = FMath::Min(segments[B].A, segments[B].B);
				return la != lb ? la < lb : A < B;
			});
			TArray<TArray<int32>> tracks;
			for (int32 s : list)
			{
				if (Cancelled())
					return false;
				const double lo = FMath::Min(segments[s].A, segments[s].B);
				const double hi = FMath::Max(segments[s].A, segments[s].B);
				int32 chosen = INDEX_NONE;
				for (int32 t = 0; t < tracks.Num() && chosen == INDEX_NONE; ++t)
				{
					bool bFree = true;
					for (int32 other : tracks[t])
					{
						const double olo = FMath::Min(segments[other].A, segments[other].B);
						const double ohi = FMath::Max(segments[other].A, segments[other].B);
						const bool bSameStem = segments[s].Source != INDEX_NONE && segments[s].Source == segments[other].Source;
						if (bSameStem && (hi <= olo + Eps || ohi <= lo + Eps))
							continue;
						if (!(hi + TrackSep <= olo || ohi + TrackSep <= lo))
						{
							bFree = false;
							break;
						}
					}
					if (bFree)
						chosen = t;
				}
				if (chosen == INDEX_NONE)
				{
					chosen = tracks.AddDefaulted();
				}
				tracks[chosen].Add(s);
				trackOf[s] = chosen;
			}
			trackCount[channel] = tracks.Num();
		}

		// Vertical layout.
		TArray<double> layerHeight, layerJog;
		layerHeight.Init(1.0, layerCount);
		layerJog.Init(0.0, layerCount);
		for (int32 v = 0; v < realCount; ++v)
		{
			layerHeight[C.Layer[v]] = FMath::Max(layerHeight[C.Layer[v]], NodeSize[C.Nodes[v]].Y);
			layerJog[C.Layer[v]] = FMath::Max(layerJog[C.Layer[v]], nodeJog[v]);
		}

		const double minChannel = FMath::Max(S.VerticalClearance, 2.0 * StubBand);
		TArray<double> layerTop, channelBase;
		layerTop.Init(0.0, layerCount);
		channelBase.Init(0.0, channelCount);
		for (int32 l = 0; l < layerCount; ++l)
		{
			if (l > 0)
			{
				const int32 tracks = trackCount[(l - 1) + 1];
				const double height = FMath::Max(minChannel, StubBand + tracks * TrackSpacing + BubbleBand + layerJog[l] + TrackSpacing);
				layerTop[l] = FMath::RoundToDouble(layerTop[l - 1] + layerHeight[l - 1] + height);
			}
			channelBase[l + 1] = layerTop[l] + layerHeight[l];
		}
		const auto trackY = [&](int32 Channel, int32 Track) -> double
		{
			if (Channel < 0)
				return -(BubbleBand + layerJog[0] + TrackSpacing) - Track * TrackSpacing;
			return channelBase[Channel + 1] + StubBand + Track * TrackSpacing;
		};
		for (int32 s = 0; s < segments.Num(); ++s)
			segments[s].Y = trackY(segments[s].Channel, trackOf[s]);

		// Node positions.
		C.NodePos.SetNum(realCount);
		C.MinX = TNumericLimits<double>::Max();
		C.MaxX = TNumericLimits<double>::Lowest();
		for (int32 v = 0; v < realCount; ++v)
		{
			C.NodePos[v] = FIntPoint(static_cast<int32>(left[v]), static_cast<int32>(layerTop[C.Layer[v]]));
			C.MinX = FMath::Min(C.MinX, left[v]);
			C.MaxX = FMath::Max(C.MaxX, left[v] + NodeSize[C.Nodes[v]].X);
		}

		// Routes.
		C.Routes.Reset();
		C.Routes.SetNum(C.Edges.Num());
		int64 totalPoints = 0;
		for (int32 le = 0; le < C.Edges.Num(); ++le)
		{
			if (Cancelled())
				return false;
			State.SetProgress(EDialogueLayoutPhase::Routing, le, C.Edges.Num());

			const int32 e = C.Edges[le];
			const int32 u = NodeLocal[S.Edges[e].From];
			const int32 v = NodeLocal[S.Edges[e].To];
			FDialogueLayoutRoute& route = C.Routes[le];
			TArray<FVector2D>& points = route.Points;
			points.Add(FVector2D(cx[u], C.NodePos[u].Y + NodeSize[C.Nodes[u]].Y));

			if (Kind[e] == EEdgeKind::Forward)
			{
				for (int32 segIndex : edgeSegments[le])
				{
					if (segIndex == INDEX_NONE)
						continue;
					points.Add(FVector2D(segments[segIndex].A, segments[segIndex].Y));
					points.Add(FVector2D(segments[segIndex].B, segments[segIndex].Y));
				}
			}
			else
			{
				route.bFeedback = true;
				const FSegment& first = segments[edgeSegments[le][0]];
				const FSegment& second = segments[edgeSegments[le][1]];
				points.Add(FVector2D(first.A, first.Y));
				points.Add(FVector2D(first.B, first.Y));
				points.Add(FVector2D(second.A, second.Y));
				points.Add(FVector2D(second.B, second.Y));
			}
			if (jogLevel[e] != INDEX_NONE)
			{
				const double jogY = C.NodePos[v].Y - JogSpacing * 0.5 - jogLevel[e] * JogSpacing;
				points.Add(FVector2D(entryX[e], jogY));
				points.Add(FVector2D(innerX[e], jogY));
				points.Add(FVector2D(innerX[e], C.NodePos[v].Y));
			}
			else
				points.Add(FVector2D(entryX[e], C.NodePos[v].Y));

			// Drop duplicate / collinear points so every route is a clean polyline.
			TArray<FVector2D> clean;
			for (const FVector2D& p : points)
			{
				if (clean.Num() > 0 && (p - clean.Last()).IsNearlyZero(Eps))
					continue;
				while (clean.Num() >= 2)
				{
					const FVector2D& a = clean[clean.Num() - 2];
					const FVector2D& b = clean.Last();
					const bool bSameX = FMath::Abs(a.X - b.X) < Eps && FMath::Abs(b.X - p.X) < Eps;
					const bool bSameY = FMath::Abs(a.Y - b.Y) < Eps && FMath::Abs(b.Y - p.Y) < Eps;
					if (!bSameX && !bSameY)
						break;
					clean.Pop();
				}
				clean.Add(p);
			}
			points = MoveTemp(clean);
			totalPoints += points.Num();
			if (totalPoints > S.Limits.MaxTotalRoutePoints)
			{
				Fail(FString::Printf(TEXT("Layout budget exhausted: more than %d route points."), S.Limits.MaxTotalRoutePoints));
				return false;
			}

			route.ControlPosition = FIntPoint(FMath::RoundToInt(entryX[e] - BubbleSize * 0.5), C.NodePos[v].Y - FMath::RoundToInt(BubbleBand + nodeJog[v]));
			for (const FVector2D& p : points)
			{
				C.MinX = FMath::Min(C.MinX, p.X);
				C.MaxX = FMath::Max(C.MaxX, p.X);
			}
		}
		C.MaxX = FMath::Max(C.MaxX, lanesRight);
		return true;
	}

	bool FEngine::Assemble(FDialogueLayoutCheckpoint& Out) const
	{
		Out.NodePositions.Init(FIntPoint::ZeroValue, S.Nodes.Num());
		Out.Routes.Reset();
		Out.Routes.SetNum(S.Edges.Num());
		Out.Crossings = 0;
		Out.Bends = 0;
		Out.TotalRouteLength = 0.0;
		Out.InputRevision = S.Revision;

		TArray<FIntPoint> offsets;
		double cursor = 0.0;
		for (int32 ci = 0; ci < Comps.Num(); ++ci)
		{
			const FComponent& C = Comps[ci];
			if (C.NodePos.Num() != C.Nodes.Num() || C.Routes.Num() != C.Edges.Num())
				return false;
			const int32 dx = FMath::RoundToInt(cursor - C.MinX);
			cursor = dx + C.MaxX + FMath::Max(S.HorizontalClearance, MinHorizontalGap);
			for (int32 i = 0; i < C.Nodes.Num(); ++i)
				Out.NodePositions[C.Nodes[i]] = C.NodePos[i] + FIntPoint(dx, 0);
			for (int32 i = 0; i < C.Edges.Num(); ++i)
			{
				FDialogueLayoutRoute route = C.Routes[i];
				route.ControlPosition += FIntPoint(dx, 0);
				for (FVector2D& p : route.Points)
					p.X += dx;
				Out.Routes[C.Edges[i]] = MoveTemp(route);
			}
			Out.Crossings += static_cast<int32>(FMath::Min<int64>(C.Crossings, MAX_int32));
		}

		// Start's top-left is the origin.
		const FIntPoint origin = Out.NodePositions[S.AnchorId];
		for (FIntPoint& p : Out.NodePositions)
			p -= origin;
		for (FDialogueLayoutRoute& route : Out.Routes)
		{
			route.ControlPosition -= origin;
			for (FVector2D& p : route.Points)
				p -= FVector2D(origin.X, origin.Y);
			for (int32 i = 1; i < route.Points.Num(); ++i)
				Out.TotalRouteLength += (route.Points[i] - route.Points[i - 1]).Size();
			Out.Bends += FMath::Max(0, route.Points.Num() - 2);
		}
		return true;
	}

	bool FEngine::PublishIfValid(bool bRequireImprovement)
	{
		State.SetProgress(EDialogueLayoutPhase::Validating, 0, 1);
		FDialogueLayoutCheckpoint checkpoint;
		if (!Assemble(checkpoint))
			return false;

		if (bRequireImprovement && checkpoint.Crossings >= PublishedCrossings)
			return true;

		TArray<FString> problems;
		if (!FMounteaLayeredLayout::Validate(S, checkpoint, problems))
		{
			for (const FString& problem : problems)
				State.AddDiagnostic(problem);
			return false;
		}

		checkpoint.Sequence = NextSequence++;
		PublishedCrossings = checkpoint.Crossings;
		State.PublishCheckpoint(MakeShared<const FDialogueLayoutCheckpoint>(MoveTemp(checkpoint)));
		return true;
	}

	bool FEngine::Execute()
	{
		if (S.Nodes.Num() == 0)
		{
			State.SetProgress(EDialogueLayoutPhase::Finished, 0, 0);
			return true;
		}

		if (!Prepare())
			return false;
		if (Cancelled())
		{
			State.SetProgress(EDialogueLayoutPhase::Cancelled, 0, 0);
			return true;
		}

		State.SetProgress(EDialogueLayoutPhase::Layering, 0, Comps.Num());
		for (int32 i = 0; i < Comps.Num(); ++i)
		{
			State.SetProgress(EDialogueLayoutPhase::Layering, i, Comps.Num());
			if (!BuildComponent(Comps[i]))
				return false;
		}
		for (FComponent& comp : Comps)
		{
			if (!BuildGeometry(comp))
			{
				if (State.GetPhase() == EDialogueLayoutPhase::Failed)
					return false;
				State.SetProgress(EDialogueLayoutPhase::Cancelled, 0, 0);
				return true;
			}
		}
		if (!PublishIfValid(false))
			return Fail(TEXT("The baseline arrangement did not pass validation; nothing was changed."));

		// Improvement: alternating median sweeps, each complete improvement is published.
		for (int32 sweep = 0; sweep < MaxCrossingSweeps; ++sweep)
		{
			if (Cancelled())
				break;
			State.SetProgress(EDialogueLayoutPhase::ReducingCrossings, sweep + 1, MaxCrossingSweeps);

			TArray<int32> changed;
			bool bAnyActive = false;
			for (int32 ci = 0; ci < Comps.Num(); ++ci)
			{
				FComponent& C = Comps[ci];
				if (C.bConverged)
					continue;
				bAnyActive = true;
				SweepOnce(C, sweep % 2 == 0);
				if (Cancelled())
					break;
				const int64 crossings = CountCrossings(C);
				if (crossings < C.Crossings)
				{
					C.Crossings = crossings;
					C.BestLayers = C.Layers;
					C.StallCount = 0;
					changed.Add(ci);
				}
				else
				{
					C.Layers = C.BestLayers;
					for (int32 l = 0; l < C.LayerCount; ++l)
						UpdatePositions(C, l);
					++C.StallCount;
				}
				C.bConverged = C.Crossings == 0 || C.StallCount >= 2;
			}
			if (Cancelled() || !bAnyActive)
				break;

			if (changed.Num() > 0)
			{
				bool bBuilt = true;
				for (int32 ci : changed)
					bBuilt &= BuildGeometry(Comps[ci]);
				if (!bBuilt)
				{
					if (State.GetPhase() == EDialogueLayoutPhase::Failed)
					{
						// Budget exhausted while improving: keep the earlier checkpoint.
						State.SetProgress(EDialogueLayoutPhase::Finished, 0, 0);
						return true;
					}
					break;
				}
				if (!PublishIfValid(true))
					break;
			}
		}

		State.SetProgress(Cancelled() ? EDialogueLayoutPhase::Cancelled : EDialogueLayoutPhase::Finished, 0, 0);
		return true;
	}
}

bool FMounteaLayeredLayout::Run(const FDialogueLayoutSnapshot& Snapshot, FDialogueLayoutTaskState& State)
{
	FEngine engine(Snapshot, State);
	const bool bOk = engine.Execute();
	State.MarkCompleted();
	return bOk;
}

bool FMounteaLayeredLayout::Validate(const FDialogueLayoutSnapshot& Snapshot, const FDialogueLayoutCheckpoint& Checkpoint, TArray<FString>& OutProblems)
{
	const int32 startProblems = OutProblems.Num();
	const auto report = [&OutProblems](const FString& Message)
	{
		if (OutProblems.Num() < 16)
			OutProblems.Add(Message);
	};

	const int32 nodeCount = Snapshot.Nodes.Num();
	const int32 edgeCount = Snapshot.Edges.Num();
	if (Checkpoint.NodePositions.Num() != nodeCount || Checkpoint.Routes.Num() != edgeCount)
	{
		report(TEXT("Validation: checkpoint does not match the snapshot."));
		return false;
	}

	// Construction-derived bound: nothing can legitimately lie further than the sum of all extents.
	double bound = 1000.0;
	for (const FDialogueLayoutNode& node : Snapshot.Nodes)
		bound += FMath::Max(node.Size.X, FallbackNodeWidth) + FMath::Max(node.Size.Y, FallbackNodeHeight) + Snapshot.HorizontalClearance + Snapshot.VerticalClearance + 2.0 * BubblePitch;
	bound += static_cast<double>(edgeCount) * (LaneSpacing + TrackSpacing + BubblePitch);
	bound = FMath::Min(bound * 2.0, 1.0e8);

	if (Snapshot.AnchorId >= 0 && Snapshot.AnchorId < nodeCount && Checkpoint.NodePositions[Snapshot.AnchorId] != FIntPoint::ZeroValue)
		report(TEXT("Validation: the Start node is not at (0,0)."));

	TArray<FRect> nodeRects;
	nodeRects.SetNum(nodeCount);
	TArray<FRect> all;
	for (int32 i = 0; i < nodeCount; ++i)
	{
		FVector2D size = Snapshot.Nodes[i].Size;
		if (!(size.X > 0.0 && size.Y > 0.0 && FMath::IsFinite(size.X) && FMath::IsFinite(size.Y)))
			size = FVector2D(FallbackNodeWidth, FallbackNodeHeight);
		const FIntPoint p = Checkpoint.NodePositions[i];
		if (FMath::Abs(static_cast<double>(p.X)) > bound || FMath::Abs(static_cast<double>(p.Y)) > bound)
			report(FString::Printf(TEXT("Validation: node %d lies outside the permitted bounds."), i));
		nodeRects[i] = {static_cast<double>(p.X), static_cast<double>(p.Y), p.X + size.X, p.Y + size.Y, i, false};
		all.Add(nodeRects[i]);
	}

	int64 totalPoints = 0;
	TArray<int32> controlRect; // edge -> index in `all`
	controlRect.Init(INDEX_NONE, edgeCount);
	for (int32 e = 0; e < edgeCount; ++e)
	{
		const FDialogueLayoutEdge& edge = Snapshot.Edges[e];
		const FDialogueLayoutRoute& route = Checkpoint.Routes[e];
		const bool bValidEdge = edge.From >= 0 && edge.From < nodeCount && edge.To >= 0 && edge.To < nodeCount;
		if (!bValidEdge)
			continue;
		if (!route.IsValid())
		{
			report(FString::Printf(TEXT("Validation: connection %d has no route."), e));
			continue;
		}
		totalPoints += route.Points.Num();
		for (const FVector2D& p : route.Points)
		{
			if (!FMath::IsFinite(p.X) || !FMath::IsFinite(p.Y) || FMath::Abs(p.X) > bound || FMath::Abs(p.Y) > bound)
			{
				report(FString::Printf(TEXT("Validation: connection %d has a non-finite or out-of-bounds point."), e));
				break;
			}
		}
		const FRect& source = nodeRects[edge.From];
		const FRect& target = nodeRects[edge.To];
		const FVector2D& first = route.Points[0];
		const FVector2D& last = route.Points.Last();
		if (FMath::Abs(first.Y - source.B) > 0.5 || first.X < source.L - 0.5 || first.X > source.R + 0.5)
			report(FString::Printf(TEXT("Validation: connection %d does not start at its source node."), e));
		if (FMath::Abs(last.Y - target.T) > 0.5 || last.X < target.L - 0.5 || last.X > target.R + 0.5)
			report(FString::Printf(TEXT("Validation: connection %d does not end at its target node."), e));

		controlRect[e] = all.Num();
		all.Add({static_cast<double>(route.ControlPosition.X), static_cast<double>(route.ControlPosition.Y),
			route.ControlPosition.X + BubbleSize, route.ControlPosition.Y + BubbleSize, e, true});
	}
	if (totalPoints > Snapshot.Limits.MaxTotalRoutePoints)
		report(TEXT("Validation: route point budget exceeded."));

	// Node/control overlap (sweep over left edges).
	{
		TArray<int32> order;
		order.Reserve(all.Num());
		for (int32 i = 0; i < all.Num(); ++i)
			order.Add(i);
		order.Sort([&all](int32 A, int32 B) { return all[A].L != all[B].L ? all[A].L < all[B].L : A < B; });
		TArray<int32> active;
		for (int32 index : order)
		{
			const FRect& current = all[index];
			for (int32 k = active.Num() - 1; k >= 0; --k)
				if (all[active[k]].R <= current.L + 0.01)
					active.RemoveAtSwap(k);
			for (int32 other : active)
			{
				const FRect& o = all[other];
				if (FMath::Min(o.B, current.B) - FMath::Max(o.T, current.T) > 0.01)
					report(FString::Printf(TEXT("Validation: %s %d overlaps %s %d."), o.bControl ? TEXT("control") : TEXT("node"), o.Owner, current.bControl ? TEXT("control") : TEXT("node"), current.Owner));
			}
			active.Add(index);
		}
	}

	// Routes must not run through unrelated node / control interiors.
	{
		TArray<FRect> shrunk = all;
		for (FRect& r : shrunk)
		{
			r.L += 0.5; r.T += 0.5; r.R -= 0.5; r.B -= 0.5;
		}
		FRectGrid grid(shrunk);
		TArray<int32> candidates;
		for (int32 e = 0; e < edgeCount; ++e)
		{
			const FDialogueLayoutRoute& route = Checkpoint.Routes[e];
			if (!route.IsValid())
				continue;
			bool bReported = false;
			for (int32 i = 1; i < route.Points.Num() && !bReported; ++i)
			{
				const FVector2D& a = route.Points[i - 1];
				const FVector2D& b = route.Points[i];
				grid.Query(FMath::Min(a.X, b.X), FMath::Min(a.Y, b.Y), FMath::Max(a.X, b.X), FMath::Max(a.Y, b.Y), candidates);
				for (int32 index : candidates)
				{
					if (index == controlRect[e])
						continue; // a connection runs through its own control
					const FRect& r = shrunk[index];
					if (r.R <= r.L || r.B <= r.T)
						continue;
					if (ClipSegmentToRect(a, b, r))
					{
						report(FString::Printf(TEXT("Validation: connection %d runs through %s %d."), e, r.bControl ? TEXT("control") : TEXT("node"), r.Owner));
						bReported = true;
						break;
					}
				}
			}
		}
	}

	return OutProblems.Num() == startProblems;
}

bool FMounteaLayeredLayout::IsRouteClear(const TArray<FVector2D>& Points, const TArray<FBox2D>& Obstacles)
{
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		if (!FMath::IsFinite(Points[i].X) || !FMath::IsFinite(Points[i].Y))
			return false;
		for (const FBox2D& box : Obstacles)
		{
			const FRect rect = {box.Min.X + 0.5, box.Min.Y + 0.5, box.Max.X - 0.5, box.Max.Y - 0.5};
			if (rect.R > rect.L && rect.B > rect.T && ClipSegmentToRect(Points[i - 1], Points[i], rect))
				return false;
		}
	}
	return true;
}
