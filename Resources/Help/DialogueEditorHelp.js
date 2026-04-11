(function () {
	function emitMessage(typeValue, targetValue) {
		if (!typeValue) {
			return;
		}

		const finalTarget = targetValue || "";
		console.log("MDS_LINK:" + typeValue + ":" + finalTarget);
	}

	document.addEventListener("click", function (eventValue) {
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
	});

	window.addEventListener("popstate", function () {
		window.history.pushState(null, null, window.location.href);
	});
})();
