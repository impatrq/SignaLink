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

document.addEventListener("DOMContentLoaded", () => {
  if (window.location.hash) {
    const targetSection = document.querySelector(window.location.hash);
    if (targetSection) {
      setTimeout(() => {
        const offsetTop = targetSection.offsetTop - 70;
        window.scrollTo({ top: offsetTop, behavior: "smooth" });
      }, 300);
    }
  }
});

document.addEventListener("DOMContentLoaded", () => {
  // --- Código original ---
  const navbar = document.getElementById("navbar");
  const mobileMenuButton = document.getElementById("mobile-menu-button");
  const mobileMenu = document.getElementById("mobile-menu");
  const navLinks = document.querySelectorAll(".nav-link");
  const mobileNavLinks = document.querySelectorAll(".mobile-nav-link");
  const sections = document.querySelectorAll("section");
  const bgElements = document.querySelectorAll(".fixed > div");

  // Toggle del menú mobile
  mobileMenuButton.addEventListener("click", () => {
    mobileMenuButton.classList.toggle("active");
    if (mobileMenu.classList.contains("open")) {
      mobileMenu.style.height = "0";
      mobileMenu.classList.remove("open");
    } else {
      mobileMenu.classList.add("open");
      mobileMenu.style.height = `${mobileMenu.scrollHeight}px`;
    }
  });

  mobileNavLinks.forEach((link) => {
    link.addEventListener("click", () => {
      mobileMenuButton.classList.remove("active");
      mobileMenu.style.height = "0";
      mobileMenu.classList.remove("open");
    });
  });

  navLinks.forEach((link) => {
    link.addEventListener("click", (e) => {
      const href = link.getAttribute("href");
      if (href && href.startsWith("#")) {
        e.preventDefault();
        const targetSection = document.querySelector(href);
        if (targetSection) {
          const offsetTop = targetSection.offsetTop - 70;
          window.scrollTo({ top: offsetTop, behavior: "smooth" });
          targetSection.classList.add("section-highlight");
          setTimeout(
            () => targetSection.classList.remove("section-highlight"),
            1000
          );
        }
      }
    });
  });

  function highlightCurrentSection() {
    let current = "";
    sections.forEach((section) => {
      const sectionTop = section.offsetTop - 100;
      const sectionHeight = section.offsetHeight;
      if (
        window.scrollY >= sectionTop &&
        window.scrollY < sectionTop + sectionHeight
      ) {
        current = section.getAttribute("id");
      }
    });
    navLinks.forEach((link) => {
      link.classList.remove("active");
      if (link.getAttribute("href") === `#${current}`) {
        link.classList.add("active");
      }
    });
    mobileNavLinks.forEach((link) => {
      link.classList.remove("active");
      if (link.getAttribute("href") === `#${current}`) {
        link.classList.add("active");
      }
    });
  }

  const observer = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          entry.target.classList.add("section-visible");
        }
      });
    },
    { threshold: 0.1 }
  );

  sections.forEach((section) => {
    section.classList.add("section-hidden");
    observer.observe(section);
  });

  setTimeout(() => {
    const headerText = document.querySelector(".text-6xl");
    if (headerText) {
      headerText.style.opacity = 1;
      headerText.style.transform = "translateY(0)";
    }
  }, 300);

  highlightCurrentSection();

  // --- Envío del formulario a Resend ---
  const form = document.querySelector("form");
  if (form) {
    form.addEventListener("submit", async (e) => {
      e.preventDefault();

      const name = document.querySelector("#name").value;
      const email = document.querySelector("#email").value;
      const subject = document.querySelector("#subject").value;
      const message = document.querySelector("#message").value;

      try {
        const res = await fetch("http://localhost:3000/send", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ name, email, subject, message }),
        });

        const data = await res.json();
        if (data.success) {
          alert("✅ Mensaje enviado correctamente");
          form.reset();
        } else {
          alert("❌ Error al enviar el mensaje");
        }
      } catch (err) {
        alert("❌ Error de conexión con el servidor");
        console.error(err);
      }
    });
  }
});

window.onload = () => {
  const text = "Tus gestos hablan, SignaLink traduce";
  const typingElement = document.getElementById("typing-text");

  if (!typingElement) return;

  const words = text.split(" ");

  setTimeout(() => {
    typingElement.style.opacity = "1";
    typingElement.innerHTML = "";

    let index = 0;

    // Pre-crear todos los spans para reservar el espacio y evitar saltos
    const spans = words.map((word, i) => {
      const span = document.createElement("span");
      span.textContent = word;
      span.style.opacity = "0";
      span.style.transition = "opacity 0.8s ease-in-out";
      span.style.display = "inline-block";
      span.style.marginRight = i < words.length - 1 ? "0.5em" : "0";
      typingElement.appendChild(span);
      return span;
    });

    // Animar la aparición de cada palabra
    const interval = setInterval(() => {
      if (index < spans.length) {
        requestAnimationFrame(() => {
          spans[index].style.opacity = "1";
        });
        index++;
      } else {
        clearInterval(interval);
      }
    }, 600);
  }, 1000);
};

// Animación de parpadeo del cursor
const style = document.createElement("style");
style.textContent = `
@keyframes blink {
  0% { opacity: 1; }
  50% { opacity: 0; }
  100% { opacity: 1; }
}
`;
document.head.appendChild(style);

// Funcionalidad de galería expandible movida desde HTML inline
function initGalleryExpandable() {
  const gallerySections = document.querySelectorAll(".gallery-section");

  gallerySections.forEach((section) => {
    const toggleBtn = section.querySelector(".expand-button");

    if (toggleBtn) {
      toggleBtn.addEventListener("click", function (e) {
        const isExpanded = section.classList.contains("expanded");

        // Cerrar todas las demás secciones
        gallerySections.forEach((otherSection) => {
          if (otherSection !== section) {
            otherSection.classList.remove("expanded");
          }
        });

        // Toggle de la sección clickeada
        section.classList.toggle("expanded");

        // Si se expande, scrollea hasta la sección
        if (section.classList.contains("expanded")) {
          section.scrollIntoView({ behavior: "smooth", block: "start" });
        }
      });

      // Que se note clickable
      toggleBtn.style.cursor = "pointer";
    }
  });
}

document.addEventListener("DOMContentLoaded", () => {
  // Inicializar funcionalidad de galería expandible
  initGalleryExpandable();
});

let slideIndex = 1;
showSlides(slideIndex);

function currentSlide(n) {
  showSlides((slideIndex = n));
}

function showSlides(n) {
  let slides = document.getElementsByClassName("carousel-slide");
  let dots = document.getElementsByClassName("dot");

  if (slides.length === 0 || dots.length === 0) {
    return;
  }

  if (n > slides.length) {
    slideIndex = 1;
  }
  if (n < 1) {
    slideIndex = slides.length;
  }

  for (let i = 0; i < slides.length; i++) {
    slides[i].classList.remove("active");
  }
  for (let i = 0; i < dots.length; i++) {
    dots[i].className = dots[i].className.replace(" active", "");
  }

  slides[slideIndex - 1].classList.add("active");
  dots[slideIndex - 1].className += " active";
}

setInterval(function () {
  showSlides((slideIndex += 1));
}, 3000);
