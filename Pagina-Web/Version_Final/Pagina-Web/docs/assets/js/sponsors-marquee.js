document.addEventListener('DOMContentLoaded', () => {
    const groups = document.querySelectorAll('.sponsors-group');
    let current = 0;
    let interval = 3500;
    let delay = 200;

    // Inicializa todos ocultos menos el primero
    groups.forEach((g, idx) => {
        g.classList.remove('active');
        g.style.zIndex = idx === current ? '2' : '1';
        const icons = g.querySelectorAll('.sponsor-icon');
        icons.forEach(icon => {
            icon.classList.remove('animate-enter', 'animate-exit', 'enter', 'exit');
            icon.style.opacity = idx === current ? '1' : '0';
        });
    });
    groups[current].classList.add('active');

    function animateRelay(prevIdx, nextIdx) {
        const prevGroup = groups[prevIdx];
        const nextGroup = groups[nextIdx];
        const prevIcons = prevGroup.querySelectorAll('.sponsor-icon');
        const nextIcons = nextGroup.querySelectorAll('.sponsor-icon');

        nextGroup.classList.add('active');
        nextGroup.style.zIndex = '2';
        prevGroup.style.zIndex = '1';

        // Aparece el siguiente sponsor justo cuando el anterior empieza a desaparecer
        prevIcons.forEach((icon, i) => {
            setTimeout(() => {
                icon.classList.remove('animate-enter');
                icon.classList.add('animate-exit');
                icon.style.opacity = '0';

                // Aparece el siguiente sponsor en relevo
                if (nextIcons[i]) {
                    nextIcons[i].classList.remove('animate-exit');
                    nextIcons[i].classList.add('animate-enter');
                    nextIcons[i].style.opacity = '1';
                }
            }, i * delay);
        });

        // Al terminar, oculta el grupo anterior y limpia clases
        setTimeout(() => {
            prevGroup.classList.remove('active');
            prevIcons.forEach(icon => {
                icon.classList.remove('animate-exit');
                icon.style.opacity = '0';
            });
        }, prevIcons.length * delay + 900);
    }

    setInterval(() => {
        let prev = current;
        current = (current + 1) % groups.length;
        animateRelay(prev, current);
    }, interval);

    // Animación inicial
    const firstIcons = groups[current].querySelectorAll('.sponsor-icon');
    firstIcons.forEach((icon, i) => {
        setTimeout(() => {
            icon.classList.add('animate-enter');
            icon.style.opacity = '1';
        }, i * delay);
    });
});