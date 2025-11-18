document.addEventListener('DOMContentLoaded', () => {
    const cards = document.querySelectorAll('.sponsor-card-vertical');

    cards.forEach(card => {
        card.addEventListener('click', () => {
            // Cierra todos
            cards.forEach(c => c.classList.remove('expanded'));
            // Expande el seleccionado
            card.classList.add('expanded');
        });
    });

    // Opcional: cerrar al hacer clic fuera
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.sponsor-card-vertical')) {
            cards.forEach(c => c.classList.remove('expanded'));
        }
    });
});