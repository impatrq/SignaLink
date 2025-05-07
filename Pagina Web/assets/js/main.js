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


  // Detectar recarga de pagina
if (performance.navigation.type === 1) {
  sessionStorage.removeItem('preloaderShown');
}

/**
 * Preloader Principal y Secundario combinados (corregido)
 */
document.addEventListener('DOMContentLoaded', function () {
  const preloader = document.getElementById('preloader');
  const circularPreloader = document.getElementById('circular-preloader');

  const hasShownPreloader = sessionStorage.getItem('preloaderShown');

  if (!hasShownPreloader && preloader) {
    // Mostrar preloader principal solo una vez
    sessionStorage.setItem('preloaderShown', 'true');

    if (circularPreloader) {
      circularPreloader.style.display = 'none'; // Asegurarse de ocultar el secundario
    }

    const canvas = document.getElementById('preloader-canvas');
    const ctx = canvas.getContext('2d');
    const loadingProgress = document.querySelector('.loading-progress');
    const loadingPercentage = document.querySelector('.loading-percentage');

    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;

    const lines = [];
    const lineCount = 50;
    for (let i = 0; i < lineCount; i++) {
      lines.push({
        x: Math.random() * canvas.width,
        y: Math.random() * canvas.height,
        length: Math.random() * 100 + 50,
        angle: Math.random() * Math.PI * 2,
        speed: Math.random() * 0.5 + 0.1,
        thickness: Math.random() * 2 + 0.5,
        color: `rgba(212, 175, 55, ${Math.random() * 0.4 + 0.1})`
      });
    }

    function animateLines() {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      lines.forEach(line => {
        ctx.beginPath();
        ctx.moveTo(line.x, line.y);
        const endX = line.x + Math.cos(line.angle) * line.length;
        const endY = line.y + Math.sin(line.angle) * line.length;
        ctx.lineTo(endX, endY);
        ctx.strokeStyle = line.color;
        ctx.lineWidth = line.thickness;
        ctx.stroke();

        line.x += Math.cos(line.angle) * line.speed;
        line.y += Math.sin(line.angle) * line.speed;

        if (line.x < -line.length || line.x > canvas.width + line.length ||
            line.y < -line.length || line.y > canvas.height + line.length) {
          line.x = Math.random() * canvas.width;
          line.y = Math.random() * canvas.height;
          line.angle = Math.random() * Math.PI * 2;
        }
      });

      if (!preloader.classList.contains('exit')) {
        requestAnimationFrame(animateLines);
      }
    }

    animateLines();

    let progress = 0;
    const loadingInterval = setInterval(() => {
      progress += Math.random() * 3 + 1;
      if (progress > 100) progress = 100;

      loadingProgress.style.width = `${progress}%`;
      loadingPercentage.textContent = `${Math.floor(progress)}%`;

      if (progress === 100) {
        clearInterval(loadingInterval);
        setTimeout(() => {
          preloader.classList.add('exit');
          setTimeout(() => {
            preloader.style.display = 'none';
          }, 500);
        }, 500);
      }
    }, 100);

    window.addEventListener('resize', () => {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
    });

  } else if (circularPreloader) {
    // Mostrar solo el preloader secundario si el principal ya se mostró
    circularPreloader.style.display = 'block';

    setTimeout(() => {
      circularPreloader.classList.add('exit');
      circularPreloader.addEventListener('transitionend', () => {
        circularPreloader.style.display = 'none';
      }, { once: true });
    }, 1000);

    if (preloader) {
      preloader.style.display = 'none'; // Asegurarse que el principal no estorbe
    }
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