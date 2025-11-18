// Lógica para el carrusel de imágenes
(function () {
  let slideIndex = 1;
  showSlides(slideIndex);

  function currentSlide(n) {
    showSlides((slideIndex = n));
  }

  function showSlides(n) {
    let i;
    let slides = document.getElementsByClassName("carousel-slide");
    let dots = document.getElementsByClassName("dot");
    if (n > slides.length) {
      slideIndex = 1;
    }
    if (n < 1) {
      slideIndex = slides.length;
    }
    for (i = 0; i < slides.length; i++) {
      slides[i].classList.remove("active");
    }
    for (i = 0; i < dots.length; i++) {
      dots[i].classList.remove("active");
    }
    slides[slideIndex - 1].classList.add("active");
    dots[slideIndex - 1].classList.add("active");
  }

var preloader = document.getElementById("preloader");

window.addEventListener("load", function () {
  setTimeout(function () {
    document.getElementById("preloader").style.display = "none";
    document.getElementById("main-content").style.display = "block";

    window.scrollTo({ top: 0, behavior: "auto" });

    // 👇 Scroll automático después del preloader
    if (window.location.hash) {
      const targetSection = document.querySelector(window.location.hash);
      if (targetSection) {
        setTimeout(() => {
          const offsetTop = targetSection.offsetTop - 70; // ajuste por navbar
          window.scrollTo({ top: offsetTop, behavior: "smooth" });
        }, 100);
      }
    }

    if (typeof Swiper !== "undefined") {
      if (window.swiperInstance) {
        window.swiperInstance.destroy(true, true);
      }
      window.swiperInstance = new Swiper(".blog-slider", {
        spaceBetween: 30,
        effect: "fade",
        loop: true,
        mousewheel: { invert: false },
        pagination: {
          el: ".blog-slider__pagination",
          clickable: true,
        },
      });
    }
  }, 1000);
});

  // Lógica para el menú móvil
  document.addEventListener("DOMContentLoaded", function () {
    const mobileMenuButton = document.getElementById("mobile-menu-button");
    const mobileMenu = document.querySelector(".nav.mobile-only");

    if (mobileMenuButton && mobileMenu) {
      mobileMenuButton.addEventListener("click", function () {
        mobileMenu.classList.toggle("active");
      });
    }
  });
})();
