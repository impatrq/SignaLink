	// Software Tarjeta Script
    document.addEventListener("DOMContentLoaded", () => {
	const cards = document.querySelectorAll(".glow-card");
	const controlBtns = document.querySelectorAll(".control-btn");
	const demoContainer = document.querySelector(".magnetic-glow-demo");

	demoContainer.classList.add("magnetic-glow");

	cards.forEach((card) => {
		card.addEventListener("mousemove", (e) => {
			if (!demoContainer.classList.contains("magnetic-glow")) return;

			const rect = card.getBoundingClientRect();
			const x = e.clientX - rect.left;
			const y = e.clientY - rect.top;

			const xPercent = Math.max(0, Math.min(100, (x / rect.width) * 100));
			const yPercent = Math.max(0, Math.min(100, (y / rect.height) * 100));

			card.querySelector(".glow-effect").style.setProperty("--x", `${xPercent}%`);
			card.querySelector(".glow-effect").style.setProperty("--y", `${yPercent}%`);
		});
	});

	controlBtns.forEach((btn) => {
		btn.addEventListener("click", () => {
			controlBtns.forEach((b) => b.classList.remove("active"));
			btn.classList.add("active");

			const effectType = btn.getAttribute("data-effect");

			demoContainer.classList.remove(
				"magnetic-glow",
				"outline-glow",
				"pulse-glow"
			);
			demoContainer.classList.add(`${effectType}-glow`);
		});
	});
})

  // Hardware Tarjeta Script
  const buttons = document.querySelectorAll('.tab-btn');
  const panes = document.querySelectorAll('.tab-pane');

  buttons.forEach(btn => {
      btn.addEventListener('click', () => {
          // reset
          buttons.forEach(b => b.classList.remove('active', 'border-yellow-500', 'bg-black'));
          panes.forEach(p => p.classList.add('hidden'));
          // activate
          btn.classList.add('active', 'border-yellow-500', 'bg-black');
          document.getElementById(btn.dataset.tab).classList.remove('hidden');
      });
  });