(function () {
	if (window.__MDS_SHARED_HELP_JS_LOCKED__) {
		return;
	}
	window.__MDS_SHARED_HELP_JS_LOCKED__ = true;

	function emitMessage(typeValue, targetValue) {
		if (!typeValue) {
			return;
		}

		const finalTarget = targetValue || "";
		console.log("MDS_LINK:" + typeValue + ":" + finalTarget);
	}

	function isEditableTarget(targetValue) {
		if (!targetValue || !(targetValue instanceof Element)) {
			return false;
		}

		const editableElement = targetValue.closest("input, textarea, [contenteditable='true']");
		return !!editableElement;
	}

	function blockEvent(eventValue) {
		eventValue.preventDefault();
		eventValue.stopPropagation();
	}

	function lockHistoryState() {
		try {
			const currentUrl = window.location.href;
			const lockState = { mdsHistoryLock: true };
			window.history.replaceState(lockState, "", currentUrl);
			window.history.pushState(lockState, "", currentUrl);
		} catch (errorValue) {
			console.log("MDS_HISTORY_LOCK_ERROR:" + String(errorValue));
		}
	}

	document.addEventListener(
		"click",
		function (eventValue) {
			const linkElement = eventValue.target.closest("a");
			if (!linkElement) {
				return;
			}

			const typeValue = linkElement.getAttribute("data-mds-type");
			const targetValue = linkElement.getAttribute("data-mds-target");
			if (!typeValue) {
				return;
			}

			eventValue.preventDefault();
			emitMessage(typeValue, targetValue);
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

	document.addEventListener(
		"mousedown",
		function (eventValue) {
			if (eventValue.button === 2 || eventValue.button === 3 || eventValue.button === 4) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	document.addEventListener(
		"pointerdown",
		function (eventValue) {
			if (eventValue.button === 2 || eventValue.button === 3 || eventValue.button === 4) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	document.addEventListener(
		"auxclick",
		function (eventValue) {
			if (eventValue.button === 3 || eventValue.button === 4) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	document.addEventListener(
		"mouseup",
		function (eventValue) {
			if (eventValue.button === 3 || eventValue.button === 4) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	window.addEventListener(
		"keydown",
		function (eventValue) {
			const isAltArrowBack =
				eventValue.altKey &&
				(eventValue.key === "ArrowLeft" || eventValue.key === "ArrowRight");
			const isBrowserBackForwardKey =
				eventValue.key === "BrowserBack" || eventValue.key === "BrowserForward";
			const isBackspaceNavigation =
				eventValue.key === "Backspace" && !isEditableTarget(eventValue.target);

			if (isAltArrowBack || isBrowserBackForwardKey || isBackspaceNavigation) {
				blockEvent(eventValue);
			}
		},
		{ capture: true, passive: false }
	);

	window.addEventListener("popstate", function () {
		lockHistoryState();
	});

	window.addEventListener("hashchange", function () {
		lockHistoryState();
	});

	lockHistoryState();
})();
