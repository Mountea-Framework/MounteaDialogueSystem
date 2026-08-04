(function () {
	if (window.__MOUNTEA_SHARED_HELP_JS_LOCKED__) {
		return;
	}
	window.__MOUNTEA_SHARED_HELP_JS_LOCKED__ = true;

	function text(value) {
		return value === undefined || value === null ? "" : String(value);
	}

	function make(tagName, className, content) {
		const element = document.createElement(tagName);
		if (className) {
			element.className = className;
		}
		if (content !== undefined) {
			element.textContent = text(content);
		}
		return element;
	}

	function emitMessage(prefixValue, typeValue, targetValue) {
		if (!typeValue) {
			return;
		}

		console.log(prefixValue + "_LINK:" + typeValue + ":" + (targetValue || ""));
	}

	function isEditableTarget(targetValue) {
		if (!targetValue || !(targetValue instanceof Element)) {
			return false;
		}

		return !!targetValue.closest("input, textarea, [contenteditable='true']");
	}

	function blockEvent(eventValue) {
		eventValue.preventDefault();
		eventValue.stopPropagation();
	}

	function lockHistoryState() {
		try {
			const currentUrl = window.location.href;
			const lockState = { mounteaHistoryLock: true };
			window.history.replaceState(lockState, "", currentUrl);
			window.history.pushState(lockState, "", currentUrl);
		} catch (errorValue) {
			console.log("MOUNTEA_HISTORY_LOCK_ERROR:" + String(errorValue));
		}
	}

	function statusBadgeClass(statusValue) {
		switch (statusValue) {
			case "Added":
				return "setup-badge setup-badge--added";
			case "Already Present":
				return "setup-badge setup-badge--present";
			case "Failed":
				return "setup-badge setup-badge--failed";
			case "Native Class":
				return "setup-badge setup-badge--cpp";
			default:
				return "setup-badge setup-badge--skipped";
		}
	}

	function statusSummaryClass(reportValue) {
		const metrics = reportValue.metrics || {};
		if ((metrics.failed || 0) > 0) {
			return "setup-summary--failed";
		}
		if ((metrics.warnings || 0) > 0) {
			return "setup-summary--partial";
		}
		return "setup-summary--success";
	}

	function parseMaisSetupReport() {
		const dataElement = document.getElementById("mais-setup-report-data");
		if (!dataElement) {
			return null;
		}

		try {
			return JSON.parse(dataElement.textContent || "{}");
		} catch (errorValue) {
			console.warn("Unable to parse MAIS setup report.", errorValue);
			return {};
		}
	}

	function renderChip(containerValue, labelValue, valueValue, classNameValue) {
		if (!containerValue) {
			return;
		}

		containerValue.appendChild(make("span", "setup-chip ui-chip " + (classNameValue || ""), labelValue + ": " + text(valueValue)));
	}

	function renderSetupRows(containerIdValue, areaValue, resultsValue) {
		const container = document.getElementById(containerIdValue);
		if (!container) {
			return;
		}

		const results = resultsValue || [];
		if (results.length === 0) {
			const row = make("tr");
			const cell = make("td", "", "No items were reported for this section.");
			cell.colSpan = 4;
			row.appendChild(cell);
			container.appendChild(row);
			return;
		}

		results.forEach(function (resultValue) {
			const row = make("tr");
			row.appendChild(make("td", "", areaValue));
			row.appendChild(make("td", "cell-class", resultValue.itemName || "Item"));
			const statusCell = make("td");
			statusCell.appendChild(make("span", statusBadgeClass(resultValue.status), resultValue.status || "Skipped"));
			row.appendChild(statusCell);
			row.appendChild(make("td", "", resultValue.message || ""));
			container.appendChild(row);
		});
	}

	function renderMaisSetupReport() {
		const report = parseMaisSetupReport();
		if (!report) {
			return;
		}

		const summary = document.getElementById("setup-summary");
		if (summary) {
			summary.textContent = report.summary || "Setup Defaults report complete.";
			summary.className = "setup-summary-message " + statusSummaryClass(report);
		}

		const metrics = report.metrics || {};
		const metricsContainer = document.getElementById("setup-metrics");
		renderChip(metricsContainer, "Added", metrics.added || 0, "setup-chip--success");
		renderChip(metricsContainer, "Already Present", metrics.alreadyPresent || 0, "setup-chip--success");
		renderChip(metricsContainer, "Warnings", metrics.warnings || 0, "setup-chip--partial");
		renderChip(metricsContainer, "Failed", metrics.failed || 0, "setup-chip--failed");

		const context = report.context || {};
		const contextContainer = document.getElementById("setup-context");
		renderChip(contextContainer, "GameMode", context.gameMode || "Not resolved", "setup-chip--gamemode");
		renderChip(contextContainer, "Source", context.gameModeSource || "World GameMode", "setup-chip--gamemode");
		renderChip(contextContainer, "Player", context.playerClass || "Not resolved", "setup-chip--gamemode");
		renderChip(contextContainer, "Settings Saved", context.settingsSaved ? "Yes" : "No", context.settingsSaved ? "setup-chip--success" : "setup-chip--partial");

		renderSetupRows("settings-results", "Settings", report.settingsResults);
		renderSetupRows("player-results", "Player", report.playerResults);
	}

	document.addEventListener(
		"click",
		function (eventValue) {
			const mdsLink = eventValue.target.closest("a[data-mds-type]");
			const inventoryLink = eventValue.target.closest("a[data-type]");
			const linkElement = mdsLink || inventoryLink;
			if (!linkElement) {
				return;
			}

			eventValue.preventDefault();
			if (mdsLink) {
				emitMessage("MDS", linkElement.getAttribute("data-mds-type"), linkElement.getAttribute("data-mds-target"));
				return;
			}

			emitMessage("MIAE", linkElement.getAttribute("data-type"), linkElement.getAttribute("href"));
		},
		false
	);

	document.addEventListener(
		"contextmenu",
		function (eventValue) {
			blockEvent(eventValue);
		},
		{ capture: true, passive: false }
	);

	["mousedown", "pointerdown", "auxclick", "mouseup"].forEach(function (eventNameValue) {
		document.addEventListener(
			eventNameValue,
			function (eventValue) {
				if (eventValue.button === 2 || eventValue.button === 3 || eventValue.button === 4) {
					blockEvent(eventValue);
				}
			},
			{ capture: true, passive: false }
		);
	});

	document.addEventListener(
		"keydown",
		function (eventValue) {
			const isAltArrowBack = eventValue.altKey && (eventValue.key === "ArrowLeft" || eventValue.key === "ArrowRight");
			const isBrowserBackForwardKey = eventValue.key === "BrowserBack" || eventValue.key === "BrowserForward";
			const isBackspaceNavigation = eventValue.key === "Backspace" && !isEditableTarget(eventValue.target);
			if (isAltArrowBack || isBrowserBackForwardKey || isBackspaceNavigation) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	document.querySelectorAll("[data-toggle]").forEach(function (toggleValue) {
		toggleValue.addEventListener("click", function () {
			const endpoint = toggleValue.closest("[data-endpoint]");
			const body = endpoint ? endpoint.querySelector(".endpoint-body") : null;
			if (!body) {
				return;
			}

			const isOpen = body.classList.contains("open");
			body.classList.toggle("open", !isOpen);
			toggleValue.textContent = isOpen ? "Show details" : "Hide details";
		});
	});

	window.addEventListener("popstate", lockHistoryState);
	window.addEventListener("hashchange", lockHistoryState);

	lockHistoryState();
	renderMaisSetupReport();
})();
