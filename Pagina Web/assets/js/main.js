/**
* Template Name: Gp
* Template URL: https://bootstrapmade.com/gp-free-multipurpose-html-bootstrap-template/
* Updated: Aug 15 2024 with Bootstrap v5.3.3
* Author: BootstrapMade.com
* License: https://bootstrapmade.com/license/
*/

(function() {
  "use strict";

  /**
   * Apply .scrolled class to the body as the page is scrolled down
   */
  function toggleScrolled() {
    const selectBody = document.querySelector('body');
    const selectHeader = document.querySelector('#header');
    if (!selectHeader.classList.contains('scroll-up-sticky') && !selectHeader.classList.contains('sticky-top') && !selectHeader.classList.contains('fixed-top')) return;
    window.scrollY > 100 ? selectBody.classList.add('scrolled') : selectBody.classList.remove('scrolled');
  }

  document.addEventListener('scroll', toggleScrolled);
  window.addEventListener('load', toggleScrolled);

  /**
   * Mobile nav toggle
   */
  const mobileNavToggleBtn = document.querySelector('.mobile-nav-toggle');

  function mobileNavToogle() {
    document.querySelector('body').classList.toggle('mobile-nav-active');
    mobileNavToggleBtn.classList.toggle('bi-list');
    mobileNavToggleBtn.classList.toggle('bi-x');
  }
  if (mobileNavToggleBtn) {
    mobileNavToggleBtn.addEventListener('click', mobileNavToogle);
  }

  /**
   * Hide mobile nav on same-page/hash links
   */
  document.querySelectorAll('#navmenu a').forEach(navmenu => {
    navmenu.addEventListener('click', () => {
      if (document.querySelector('.mobile-nav-active')) {
        mobileNavToogle();
      }
    });

  });

  /**
   * Toggle mobile nav dropdowns
   */
  document.querySelectorAll('.navmenu .toggle-dropdown').forEach(navmenu => {
    navmenu.addEventListener('click', function(e) {
      e.preventDefault();
      this.parentNode.classList.toggle('active');
      this.parentNode.nextElementSibling.classList.toggle('dropdown-active');
      e.stopImmediatePropagation();
    });
  });

/**
 * Preloader
 */
window.addEventListener('load', () => {
  const preloader = document.getElementById('preloader');
  const mainContent = document.getElementById('main-content');
  
  // Verificar si estamos en la página principal (index.html)
  const isIndexPage = window.location.pathname.endsWith('index.html') || 
                      window.location.pathname.endsWith('/') || 
                      window.location.pathname.split('/').pop() === '';
  
  // Verificar si existe el preloader personalizado (solo en index.html)
  const logoContainer = document.getElementById('logo-container');
  const textContainer = document.getElementById('text-container');
  
  if (isIndexPage && logoContainer && textContainer) {
    // Estamos en index.html con el preloader personalizado
    const titleContainer = document.getElementById('title-container');
    const phraseContainer = document.getElementById('phrase-container');
    const canvas = document.getElementById('circuit-canvas');
    const topLine = document.getElementById('top-line');
    const bottomLine = document.getElementById('bottom-line');
    const loadingText = document.getElementById('loading-text');
    
    // Elementos de título y frase
    const titleLetters = document.querySelectorAll('.title-letter');
    const phraseWords = document.querySelectorAll('.phrase-word');
    
    // Configurar canvas
    setupCanvas();
    
    // Iniciar secuencia de animación
    startAnimationSequence();
    
    // Función para configurar el canvas
    function setupCanvas() {
      const ctx = canvas.getContext('2d');
      
      // Ajustar tamaño del canvas
      function resizeCanvas() {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        drawCircuitElements();
      }
      
      // Dibujar elementos de circuito
      function drawCircuitElements() {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.strokeStyle = '#D4AF37';
        ctx.fillStyle = '#D4AF37';
        
        // Dibujar puntos
        for (let i = 0; i < 50; i++) {
          const x = Math.random() * canvas.width;
          const y = Math.random() * canvas.height;
          const size = Math.random() * 3 + 1;
          
          ctx.beginPath();
          ctx.arc(x, y, size, 0, Math.PI * 2);
          ctx.fill();
        }
        
        // Dibujar líneas horizontales
        for (let i = 0; i < 15; i++) {
          const y = Math.random() * canvas.height;
          const x = Math.random() * canvas.width;
          const length = Math.random() * 100 + 50;
          
          ctx.beginPath();
          ctx.moveTo(x, y);
          ctx.lineTo(x + length, y);
          ctx.lineWidth = 1;
          ctx.stroke();
        }
        
        // Dibujar líneas verticales
        for (let i = 0; i < 15; i++) {
          const x = Math.random() * canvas.width;
          const y = Math.random() * canvas.height;
          const length = Math.random() * 100 + 50;
          
          ctx.beginPath();
          ctx.moveTo(x, y);
          ctx.lineTo(x, y + length);
          ctx.lineWidth = 1;
          ctx.stroke();
        }
      }
      
      window.addEventListener('resize', resizeCanvas);
      resizeCanvas();
    }
    
    // Función para iniciar la secuencia de animación
    function startAnimationSequence() {
      // Mostrar contenedor de texto después de 1.5 segundos y ocultar logo
      setTimeout(() => {
        // Hacer que el logo desaparezca
        logoContainer.classList.add('fade-out');
        
        // Mostrar el contenedor de texto
        setTimeout(() => {
          textContainer.classList.remove('hidden');
          textContainer.classList.add('visible');
          
          // Mostrar línea superior
          setTimeout(() => {
            topLine.classList.add('visible');
          }, 200);
          
          // Animar cada letra del título
          titleLetters.forEach((letter, index) => {
            setTimeout(() => {
              letter.style.opacity = '1';
              letter.style.transform = 'translateY(0)';
              letter.style.filter = 'blur(0px)';
              letter.style.transition = 'opacity 0.6s ease, transform 0.6s cubic-bezier(0.22, 1, 0.36, 1), filter 0.6s ease';
            }, 400 + (80 * index));
          });
          
          // Animar cada palabra de la frase
          phraseWords.forEach((word, index) => {
            setTimeout(() => {
              word.style.opacity = '1';
              word.style.transform = 'translateY(0)';
              word.style.transition = 'opacity 0.5s ease, transform 0.5s cubic-bezier(0.22, 1, 0.36, 1)';
            }, 1200 + (100 * index));
          });
          
          // Mostrar línea inferior después de que aparezca la frase
          setTimeout(() => {
            bottomLine.classList.add('visible');
          }, 1800);
          
          // Mostrar texto de carga
          setTimeout(() => {
            loadingText.classList.remove('hidden');
            loadingText.style.opacity = '1';
            loadingText.style.transition = 'opacity 0.8s ease';
          }, 2200);
        }, 400); // Pequeño retraso para que el logo desaparezca primero
        
      }, 1500);
      
      // Finalizar preloader después de 5.5 segundos
      setTimeout(() => {
        preloader.style.transition = 'opacity 1s ease, transform 0.8s cubic-bezier(0.65, 0, 0.35, 1)';
        preloader.style.opacity = '0';
        preloader.style.transform = 'translateY(-100%)';
        
        // Mostrar contenido principal después de que termine la animación de salida
        setTimeout(() => {
          preloader.style.display = 'none';
          if (mainContent) {
            mainContent.style.display = 'block';
          }
        }, 800);
      }, 5500);
    }
  } else {
    // Estamos en una página secundaria con el preloader simple
    setTimeout(() => {
      preloader.style.transition = 'opacity 1s ease';
      preloader.style.opacity = '0';

      setTimeout(() => {
        preloader.style.display = 'none';
        if (mainContent) {
          mainContent.style.display = 'block';
        }
      }, 1000);
    }, 2000);
  }
});

  /**
   * Scroll top button
   */
  let scrollTop = document.querySelector('.scroll-top');

  function toggleScrollTop() {
    if (scrollTop) {
      window.scrollY > 100 ? scrollTop.classList.add('active') : scrollTop.classList.remove('active');
    }
  }
  scrollTop.addEventListener('click', (e) => {
    e.preventDefault();
    window.scrollTo({
      top: 0,
      behavior: 'smooth'
    });
  });

  window.addEventListener('load', toggleScrollTop);
  document.addEventListener('scroll', toggleScrollTop);

  /**
   * Animation on scroll function and init
   */
  function aosInit() {
    AOS.init({
      duration: 600,
      easing: 'ease-in-out',
      once: true,
      mirror: false
    });
  }
  window.addEventListener('load', aosInit);

  /**
   * Init swiper sliders
   */
  function initSwiper() {
    document.querySelectorAll(".init-swiper").forEach(function(swiperElement) {
      let config = JSON.parse(
        swiperElement.querySelector(".swiper-config").innerHTML.trim()
      );

      if (swiperElement.classList.contains("swiper-tab")) {
        initSwiperWithCustomPagination(swiperElement, config);
      } else {
        new Swiper(swiperElement, config);
      }
    });
  }

  window.addEventListener("load", initSwiper);

  /**
   * Initiate glightbox
   */
  const glightbox = GLightbox({
    selector: '.glightbox'
  });

  /**
   * Init isotope layout and filters
   */
  document.querySelectorAll('.isotope-layout').forEach(function(isotopeItem) {
    let layout = isotopeItem.getAttribute('data-layout') ?? 'masonry';
    let filter = isotopeItem.getAttribute('data-default-filter') ?? '*';
    let sort = isotopeItem.getAttribute('data-sort') ?? 'original-order';

    let initIsotope;
    imagesLoaded(isotopeItem.querySelector('.isotope-container'), function() {
      initIsotope = new Isotope(isotopeItem.querySelector('.isotope-container'), {
        itemSelector: '.isotope-item',
        layoutMode: layout,
        filter: filter,
        sortBy: sort
      });
    });

    isotopeItem.querySelectorAll('.isotope-filters li').forEach(function(filters) {
      filters.addEventListener('click', function() {
        isotopeItem.querySelector('.isotope-filters .filter-active').classList.remove('filter-active');
        this.classList.add('filter-active');
        initIsotope.arrange({
          filter: this.getAttribute('data-filter')
        });
        if (typeof aosInit === 'function') {
          aosInit();
        }
      }, false);
    });

  });

  /**
   * Initiate Pure Counter
   */
  new PureCounter();

  /**
   * Correct scrolling position upon page load for URLs containing hash links.
   */
  window.addEventListener('load', function(e) {
    if (window.location.hash) {
      if (document.querySelector(window.location.hash)) {
        setTimeout(() => {
          let section = document.querySelector(window.location.hash);
          let scrollMarginTop = getComputedStyle(section).scrollMarginTop;
          window.scrollTo({
            top: section.offsetTop - parseInt(scrollMarginTop),
            behavior: 'smooth'
          });
        }, 100);
      }
    }
  });

  window.addEventListener('scroll', function() {
    const navbar = document.querySelector('.header');
    if (window.scrollY > 10) {
      navbar.classList.add('scrolled');
    } else {
      navbar.classList.remove('scrolled');
    }
  });

  /**
   * Navmenu Scrollspy
   */
  let navmenulinks = document.querySelectorAll('.navmenu a');

  function navmenuScrollspy() {
    navmenulinks.forEach(navmenulink => {
      if (!navmenulink.hash) return;
      let section = document.querySelector(navmenulink.hash);
      if (!section) return;
      let position = window.scrollY + 200;
      if (position >= section.offsetTop && position <= (section.offsetTop + section.offsetHeight)) {
        document.querySelectorAll('.navmenu a.active').forEach(link => link.classList.remove('active'));
        navmenulink.classList.add('active');
      } else {
        navmenulink.classList.remove('active');
      }
    })
  }
  window.addEventListener('load', navmenuScrollspy);
  document.addEventListener('scroll', navmenuScrollspy);

})();