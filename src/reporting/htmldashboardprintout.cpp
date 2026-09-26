///////////////////////////////////////////////////////////////////////////////
// Name:        htmldashboardprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmldashboardprintout.h"
#include "svgreportprintout.h"
#include <algorithm>
#include <wx/app.h>
#include <wx/file.h>
#include <wx/msgdlg.h>

//------------------------------------------------------
static wxString GetDashboardScriptState()
    {
    return LR"JS(
(function() {
  const root = document.documentElement;
  const strings = {{STRINGS}};
  const colorKey = 'wisteria-dashboard-color-mode';
  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  let colorMode = '{{MODE}}';
  const hasModeToggle = {{TOGGLE}};
  const countUp = {{COUNTUP}};
  const layers = {{LAYERS}};
  const activeLayers = new Set(layers);
  let view = '{{VIEW}}';
  let current = 0;
  let pages = [];

  if (hasModeToggle) {
    try {
      const saved = localStorage.getItem(colorKey);
      if (saved === 'auto' || saved === 'light' || saved === 'dark') colorMode = saved;
    } catch (e) {}
  }

  function schemeFor(value) {
    return value === 'auto' ? 'light dark' : value;
  }
  function applyColorMode() {
    root.style.colorScheme = schemeFor(colorMode);
    document.querySelectorAll('.dash-modes button').forEach(function(btn) {
      btn.setAttribute('aria-pressed', btn.dataset.mode === colorMode ? 'true' : 'false');
    });
  }
  function setColorMode(value) {
    colorMode = value;
    applyColorMode();
    try { localStorage.setItem(colorKey, value); } catch (e) {}
  }
  function format(template) {
    const args = Array.prototype.slice.call(arguments, 1);
    return template.replace(/\{(\d+)\}/g, function(match, index) { return args[index]; });
  }
  function announce(text) {
    const el = document.getElementById('dash-status');
    if (el) { el.textContent = ''; el.textContent = text; }
  }
  root.style.colorScheme = schemeFor(colorMode);
  if (!reduceMotion.matches) root.classList.add('motion-ready');
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptPages()
    {
    return LR"JS(
  function collectPages() {
    pages = Array.from(document.querySelectorAll('.page')).map(function(el, i) {
      return {
        el: el,
        layer: el.getAttribute('data-layer') || '',
        title: el.getAttribute('aria-label') || format(strings.page, i + 1)
      };
    });
  }
  function clampIndex(index) {
    return Math.max(0, Math.min(pages.length - 1, index));
  }
  function isVisible(page) {
    return !page.layer || activeLayers.has(page.layer);
  }
  function visibleIndexes() {
    const result = [];
    pages.forEach(function(page, i) { if (isVisible(page)) result.push(i); });
    return result;
  }
  function stepPage(delta) {
    const visible = visibleIndexes();
    if (!visible.length) return;
    const pos = visible.indexOf(current);
    if (pos < 0) {
      goTo(visible[0]);
      return;
    }
    goTo(visible[Math.max(0, Math.min(visible.length - 1, pos + delta))]);
  }
  function applyLayers() {
    pages.forEach(function(page) {
      const hidden = !isVisible(page);
      page.el.classList.toggle('is-filtered', hidden);
      if (page.card) page.card.classList.toggle('is-filtered', hidden);
      if (page.dot) page.dot.classList.toggle('is-filtered', hidden);
    });
    document.querySelectorAll('.dash-layers button').forEach(function(btn) {
      btn.setAttribute('aria-pressed', activeLayers.has(btn.dataset.layer) ? 'true' : 'false');
    });
  }
  function toggleLayer(layer) {
    if (activeLayers.has(layer)) activeLayers.delete(layer);
    else activeLayers.add(layer);
    applyLayers();
    const visible = visibleIndexes();
    if (visible.length && visible.indexOf(current) < 0) {
      const next = visible.find(function(i) { return i > current; });
      current = next === undefined ? visible[visible.length - 1] : next;
    }
    renderView();
    revealCurrent('auto');
    announce(visible.length ? format(strings.pagesShown, visible.length, pages.length) :
                              strings.noPages);
  }
  function readHash() {
    const params = new URLSearchParams(location.hash.slice(1));
    const wanted = params.get('view');
    if (wanted === 'atlas' || wanted === 'story') view = wanted;
    const index = parseInt(params.get('page'), 10);
    if (!isNaN(index)) current = clampIndex(index);
  }
  function writeHash() {
    try { history.replaceState(null, '', '#view=' + view + '&page=' + current); } catch (e) {}
  }
  function measureChrome() {
    const toolbar = document.querySelector('.dash-toolbar');
    if (toolbar) root.style.setProperty('--toolbar-height', toolbar.offsetHeight + 'px');
    const strip = document.getElementById('dash-atlas');
    root.style.setProperty('--strip-height',
                           view === 'atlas' && strip ? strip.offsetHeight + 'px' : '0px');
  }
  function buildAtlas() {
    const strip = document.getElementById('dash-atlas');
    if (!strip) return;
    const svgNs = 'http://www.w3.org/2000/svg';
    pages.forEach(function(page, i) {
      const card = document.createElement('button');
      card.type = 'button';
      card.className = 'dash-card';
      card.setAttribute('aria-label', format(strings.goTo, page.title));
      const thumb = document.createElementNS(svgNs, 'svg');
      thumb.setAttribute('class', 'dash-card-thumb');
      thumb.setAttribute('viewBox', page.el.querySelector('.page-svg').getAttribute('viewBox'));
      thumb.setAttribute('aria-hidden', 'true');
      const use = document.createElementNS(svgNs, 'use');
      use.setAttribute('href', '#page-content-' + i);
      thumb.appendChild(use);
      const label = document.createElement('span');
      label.className = 'dash-card-title';
      label.textContent = page.title;
      card.appendChild(thumb);
      card.appendChild(label);
      card.addEventListener('click', function() { goTo(i, true); });
      strip.appendChild(card);
      page.card = card;
    });
  }
  function updatePager() {
    const label = document.getElementById('dash-pager-label');
    const prev = document.getElementById('dash-prev');
    const next = document.getElementById('dash-next');
    if (!label || !pages.length) return;
    const visible = visibleIndexes();
    const pos = visible.indexOf(current);
    const text = pos < 0 ? strings.noPages :
                 (pos + 1) + ' / ' + visible.length + ' · ' + pages[current].title;
    label.textContent = text;
    label.title = text;
    prev.setAttribute('aria-disabled', pos <= 0 ? 'true' : 'false');
    next.setAttribute('aria-disabled', pos < 0 || pos >= visible.length - 1 ? 'true' : 'false');
  }
  function updateProgress() {
    if (view !== 'story') return;
    const span = document.documentElement.scrollHeight - window.innerHeight;
    root.style.setProperty('--progress', span > 0 ? String(Math.min(1, window.scrollY / span)) : '0');
  }
  function replayShown() {
    pages.forEach(function(page) {
      const shown = page.el.getClientRects().length > 0;
      if (shown && page.shown === false && page.revealed) startCounters(page);
      page.shown = shown;
    });
  }
  function renderView() {
    root.setAttribute('data-view', view);
    pages.forEach(function(page, i) {
      page.el.classList.toggle('is-current', i === current);
      if (!page.card) return;
      if (i === current) page.card.setAttribute('aria-current', 'true');
      else page.card.removeAttribute('aria-current');
    });
    replayShown();
    document.querySelectorAll('.dash-views button').forEach(function(btn) {
      btn.setAttribute('aria-pressed', btn.dataset.view === view ? 'true' : 'false');
    });
    document.querySelectorAll('.dash-rail button').forEach(function(btn, i) {
      if (i === current) btn.setAttribute('aria-current', 'true');
      else btn.removeAttribute('aria-current');
    });
    updatePager();
    updateProgress();
    measureChrome();
    writeHash();
  }
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptInk()
    {
    return LR"JS(
  function paintKind(el) {
    const holder = el.closest('[fill]');
    if (!holder) return 'none';
    const fill = holder.getAttribute('fill').toUpperCase();
    if (fill === 'NONE') return 'none';
    const opacity = parseFloat(holder.getAttribute('fill-opacity'));
    if (!isNaN(opacity) && opacity < 0.5) return 'none';
    return (fill === '#FFFFFF' || fill === '#000000' || fill === 'WHITE' || fill === 'BLACK') ?
           'page' : 'color';
  }
  function insideShape(el, x, y) {
    try {
      if (typeof el.isPointInFill !== 'function') return true;
      const matrix = el.getScreenCTM();
      if (!matrix) return true;
      return el.isPointInFill(new DOMPoint(x, y).matrixTransform(matrix.inverse()));
    } catch (e) {
      return true;
    }
  }
  // black or white text drawn on a colored shape keeps its painted color in dark mode
  function tagInk(page) {
    const shapes = [];
    const nodes = page.el.querySelectorAll('rect, path, polygon, ellipse, circle, text');
    nodes.forEach(function(el) {
      if (el.tagName !== 'text') {
        if (el.closest('defs, clipPath, pattern, marker, mask')) return;
        const kind = paintKind(el);
        if (kind === 'none') return;
        const box = el.getBoundingClientRect();
        if (box.width >= 2 && box.height >= 2) shapes.push({ el: el, box: box, kind: kind });
        return;
      }
      const fill = (el.getAttribute('fill') || '').toUpperCase();
      if (fill !== '#FFFFFF' && fill !== '#000000' && fill !== 'WHITE' && fill !== 'BLACK') return;
      const box = el.getBoundingClientRect();
      if (!box.width && !box.height) return;
      const x = box.left + box.width / 2;
      const y = box.top + box.height / 2;
      for (let i = shapes.length - 1; i >= 0; --i) {
        const shape = shapes[i];
        if (x < shape.box.left || x > shape.box.right ||
            y < shape.box.top || y > shape.box.bottom) continue;
        if (!insideShape(shape.el, x, y)) continue;
        if (shape.kind === 'color') el.classList.add('ink-keep');
        return;
      }
    });
  }
  function assignInk() {
    const hidden = pages.filter(function(page) { return page.el.getClientRects().length === 0; });
    hidden.forEach(function(page) { page.el.classList.add('ink-measure'); });
    pages.forEach(function(page) {
      try { tagInk(page); } catch (e) {}
    });
    hidden.forEach(function(page) { page.el.classList.remove('ink-measure'); });
  }
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptNavigation()
    {
    return LR"JS(
  function revealCurrent(behavior) {
    const page = pages[current];
    if (!page) return;
    if (view === 'atlas') {
      window.scrollTo({ top: 0, behavior: 'auto' });
      if (page.card) page.card.scrollIntoView({ behavior: behavior, block: 'nearest', inline: 'center' });
    } else {
      page.el.scrollIntoView({ behavior: behavior, block: 'start' });
    }
  }
  function focusPage(index) {
    const page = pages[index];
    if (page) page.el.focus({ preventScroll: true });
  }
  function withTransition(update) {
    if (!document.startViewTransition || reduceMotion.matches) {
      update();
      return;
    }
    const activeEl = pages[current] ? pages[current].el : null;
    if (activeEl) activeEl.style.viewTransitionName = 'active-page';
    const cleanup = function() { if (activeEl) activeEl.style.viewTransitionName = ''; };
    document.startViewTransition(update).finished.then(cleanup, cleanup);
  }
  function setView(next) {
    withTransition(function() {
      view = next;
      renderView();
      revealCurrent('auto');
    });
    announce(strings[next]);
  }
  function goTo(index, moveFocus) {
    if (!pages.length) return;
    current = clampIndex(index);
    if (view === 'atlas') {
      withTransition(function() {
        renderView();
        revealCurrent('auto');
        if (moveFocus) focusPage(current);
      });
    } else {
      renderView();
      revealCurrent(reduceMotion.matches ? 'auto' : 'smooth');
      arrive(pages[current]);
      if (moveFocus) focusPage(current);
    }
    const visible = visibleIndexes();
    announce(format(strings.pageOf, visible.indexOf(current) + 1, visible.length) + ': ' +
             pages[current].title);
  }
  function buildRail() {
    const rail = document.getElementById('dash-rail');
    if (!rail) return;
    pages.forEach(function(page, i) {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.title = page.title;
      btn.setAttribute('aria-label', format(strings.goTo, page.title));
      btn.addEventListener('click', function() { goTo(i, true); });
      rail.appendChild(btn);
      page.dot = btn;
    });
  }
  function observeStory() {
    if (!('IntersectionObserver' in window)) return;
    const observer = new IntersectionObserver(function(entries) {
      if (view !== 'story') return;
      entries.forEach(function(entry) {
        if (!entry.isIntersecting) return;
        const index = pages.findIndex(function(page) { return page.el === entry.target; });
        if (index >= 0 && index !== current) {
          current = index;
          renderView();
        }
      });
    }, { threshold: 0.6 });
    pages.forEach(function(page) { observer.observe(page.el); });
  }
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptCounters()
    {
    return LR"JS(
  const numberPattern = /^([$€£]?)(\d{1,3}(?:,\d{3})+|\d+)(\.\d+)?(%|[kKmMbB])?$/;
  function collectCounters(svg) {
    const texts = Array.from(svg.querySelectorAll('text'));
    const sizes = texts.map(function(el) { return parseFloat(getComputedStyle(el).fontSize) || 0; });
    const sorted = sizes.slice().sort(function(a, b) { return a - b; });
    const median = sorted.length ? sorted[Math.floor(sorted.length / 2)] : 0;
    const counters = [];
    texts.forEach(function(el, i) {
      if (el.childNodes.length !== 1 || el.firstChild.nodeType !== 3) return;
      if (sizes[i] < 20 || sizes[i] < median * 1.5) return;
      const original = el.textContent;
      const m = numberPattern.exec(original.trim());
      if (!m) return;
      const fraction = m[3] ? m[3].length - 1 : 0;
      const value = parseFloat(m[2].replace(/,/g, '') + (m[3] || ''));
      const isYear = !m[1] && !m[3] && !m[4] && m[2].length === 4 && value >= 1900 && value <= 2100;
      if (isYear || !(value > 0)) return;
      counters.push({
        el: el, original: original, value: value,
        render: function(v) {
          let text = v.toFixed(fraction);
          if (m[2].indexOf(',') >= 0) {
            const parts = text.split('.');
            parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ',');
            text = parts.join('.');
          }
          return m[1] + text + (m[4] || '');
        }
      });
    });
    return counters;
  }
  function startCounters(page) {
    if (!page.counters) return;
    page.countRun = (page.countRun || 0) + 1;
    const run = page.countRun;
    page.counters.forEach(function(counter) {
      const begin = performance.now();
      counter.el.style.fontVariantNumeric = 'tabular-nums';
      counter.el.textContent = counter.render(0);
      const frame = function(now) {
        if (run !== page.countRun) return;
        const t = Math.max(0, Math.min(1, (now - begin) / 1100));
        if (t >= 1) {
          counter.el.textContent = counter.original;
          return;
        }
        counter.el.textContent = counter.render(counter.value * (1 - Math.pow(1 - t, 3)));
        window.requestAnimationFrame(frame);
      };
      window.requestAnimationFrame(frame);
    });
  }
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptMotion()
    {
    return LR"JS(
  const shapeSelector = 'circle, ellipse, rect, path, polygon, polyline, line';
  const maxAnimatedMarks = 2500;

  function rgbKey(text) {
    const m = /rgba?\(\s*(\d+)[,\s]+(\d+)[,\s]+(\d+)/.exec(text);
    return m ? m[1] + ',' + m[2] + ',' + m[3] : '';
  }
  function isNearWhite(key) {
    const c = key.split(',');
    return +c[0] > 245 && +c[1] > 245 && +c[2] > 245;
  }
  function classifyMark(el, style, limitArea) {
    if (el.hasAttribute('transform')) return 'fade';
    try {
      const box = el.getBBox();
      if (box.width * box.height > limitArea) return '';
    } catch (e) {}
    const tag = el.tagName;
    if (tag === 'circle' || tag === 'ellipse') return 'pop';
    const isOpen = tag === 'line' || tag === 'polyline' ||
                   (tag === 'path' && !/[zZ]/.test(el.getAttribute('d') || ''));
    if (!isOpen && style.fill !== 'none') return 'grow';
    if (style.stroke === 'none' || style.strokeDasharray !== 'none') return '';
    return (tag === 'path' || tag === 'polyline' || tag === 'line') ? 'draw' : 'fade';
  }
  function registerSpot(page, el, style) {
    const key = rgbKey(style.fill);
    if (!key || isNearWhite(key)) return;
    el.classList.add('mk-spot');
    page.spotKeys.set(el, key);
    if (!page.spot.has(key)) page.spot.set(key, []);
    page.spot.get(key).push(el);
  }
  function prepareMarks(page) {
    if (page.prepared) return;
    page.prepared = true;
    const svg = page.el.querySelector('.page-svg');
    if (!svg) return;
    let fadeIndex = 0;
    const shapes = svg.querySelectorAll(shapeSelector);
    if (shapes.length <= maxAnimatedMarks) {
      const viewBox = svg.viewBox.baseVal;
      const limitArea = viewBox.width * viewBox.height * 0.2;
      page.spot = new Map();
      page.spotKeys = new WeakMap();
      let index = 0;
      shapes.forEach(function(el) {
        const style = getComputedStyle(el);
        const kind = classifyMark(el, style, limitArea);
        if (!kind) return;
        if (kind === 'draw') {
          let length = 0;
          try { length = el.getTotalLength(); } catch (e) {}
          if (!(length > 1)) return;
          el.style.setProperty('--len', String(length));
        }
        el.classList.add('mk', 'mk-' + kind);
        el.style.setProperty('--i', String(kind === 'fade' ? fadeIndex++ : index++));
        if (kind === 'pop' || kind === 'grow') registerSpot(page, el, style);
      });
    }
    svg.querySelectorAll('text').forEach(function(el) {
      el.classList.add('mk', 'mk-fade');
      el.style.setProperty('--i', String(fadeIndex++));
    });
    if (countUp) page.counters = collectCounters(svg);
  }
  function bindSpot(page) {
    if (page.spotBound || !page.spot || page.spot.size < 2) return;
    page.spotBound = true;
    const svg = page.el.querySelector('.page-svg');
    let currentKey = '';
    function setKey(key) {
      if (key === currentKey) return;
      if (currentKey) {
        page.spot.get(currentKey).forEach(function(el) { el.classList.remove('is-spot'); });
      }
      currentKey = key;
      if (key) page.spot.get(key).forEach(function(el) { el.classList.add('is-spot'); });
      svg.classList.toggle('is-spotting', !!key);
    }
    svg.addEventListener('mouseover', function(e) { setKey(page.spotKeys.get(e.target) || ''); });
    svg.addEventListener('mouseleave', function() { setKey(''); });
  }
  function revealPage(page) {
    prepareMarks(page);
    bindSpot(page);
    startCounters(page);
    const svg = page.el.querySelector('.page-svg');
    page.el.classList.add('is-revealed', 'is-entering');
    if (!svg) return;
    svg.addEventListener('animationend', function done(e) {
      if (e.target !== svg) return;
      page.el.classList.remove('is-entering');
      svg.removeEventListener('animationend', done);
    });
  }
  function arrive(page) {
    if (reduceMotion.matches || !page) return;
    const svg = page.el.querySelector('.page-svg');
    if (!svg) return;
    window.setTimeout(function() {
      page.el.classList.remove('is-arriving');
      void svg.getBoundingClientRect();
      page.el.classList.add('is-arriving');
      svg.addEventListener('animationend', function done(e) {
        if (e.target !== svg) return;
        page.el.classList.remove('is-arriving');
        svg.removeEventListener('animationend', done);
      });
    }, 450);
  }
  function setupMotion() {
    if (reduceMotion.matches) return;
    if (!('IntersectionObserver' in window)) {
      root.classList.remove('motion-ready');
      return;
    }
    const observer = new IntersectionObserver(function(entries) {
      entries.forEach(function(entry) {
        if (!entry.isIntersecting) return;
        const page = pages.find(function(p) { return p.el === entry.target; });
        if (!page || page.revealed) return;
        page.revealed = true;
        observer.unobserve(entry.target);
        revealPage(page);
      });
    }, { threshold: 0.2 });
    pages.forEach(function(page) { observer.observe(page.el); });
  }
)JS";
    }

//------------------------------------------------------
static wxString GetDashboardScriptEvents()
    {
    return LR"JS(
  function onKeyDown(e) {
    if (e.defaultPrevented || e.ctrlKey || e.metaKey || e.altKey || !pages.length) return;
    const tag = e.target && e.target.tagName;
    if (tag === 'INPUT' || tag === 'TEXTAREA' || (e.target && e.target.isContentEditable)) return;
    if (e.key === 'ArrowRight' || e.key === 'PageDown') {
      e.preventDefault();
      stepPage(1);
    } else if (e.key === 'ArrowLeft' || e.key === 'PageUp') {
      e.preventDefault();
      stepPage(-1);
    } else if (e.key === 'Home') {
      e.preventDefault();
      stepPage(-pages.length);
    } else if (e.key === 'End') {
      e.preventDefault();
      stepPage(pages.length);
    }
  }
  function bindControls() {
    document.querySelectorAll('.dash-views button').forEach(function(btn) {
      btn.addEventListener('click', function() { setView(btn.dataset.view); });
    });
    document.querySelectorAll('.dash-modes button').forEach(function(btn) {
      btn.addEventListener('click', function() { setColorMode(btn.dataset.mode); });
    });
    document.querySelectorAll('.dash-layers button').forEach(function(btn) {
      btn.addEventListener('click', function() { toggleLayer(btn.dataset.layer); });
    });
    [['dash-prev', -1], ['dash-next', 1]].forEach(function(pair) {
      const btn = document.getElementById(pair[0]);
      btn.addEventListener('click', function() {
        if (btn.getAttribute('aria-disabled') !== 'true') stepPage(pair[1]);
      });
    });
    document.addEventListener('keydown', onKeyDown);
    window.addEventListener('scroll', updateProgress, { passive: true });
    window.addEventListener('resize', measureChrome);
  }
  document.addEventListener('DOMContentLoaded', function() {
    collectPages();
    readHash();
    applyColorMode();
    buildRail();
    buildAtlas();
    bindControls();
    applyLayers();
    assignInk();
    renderView();
    revealCurrent('auto');
    setupMotion();
    observeStory();
  });
})();
)JS";
    }

//------------------------------------------------------
Wisteria::HtmlDashboardPrintout::HtmlDashboardPrintout(const std::vector<Canvas*>& canvases,
                                                       HtmlDashboardOptions options)
    {
    for (auto* canvas : canvases)
        {
        if (canvas != nullptr)
            {
            canvas->ApplyAutoAccessibilityAttributes();
            }
        }

    const auto escapeAttr = [](const wxString& str)
    { return SVGReportPrintout::EscapeXmlAttr(str); };
    const auto escapeText = [](const wxString& str)
    { return SVGReportPrintout::EscapeXmlText(str); };
    const auto jsString = [](const wxString& str)
    { return L"'" + SVGReportPrintout::EscapeJsString(str) + L"'"; };

    wxString title{ options.m_title };
    if (title.empty())
        {
        for (const auto* canvas : canvases)
            {
            if (canvas != nullptr && !canvas->GetLabel().empty())
                {
                title = canvas->GetLabel();
                break;
                }
            }
        }
    if (title.empty())
        {
        title = _(L"Dashboard");
        }

    wxSize pageSize{ options.m_pageSize };
    if (pageSize.GetWidth() <= 0 || pageSize.GetHeight() <= 0)
        {
        pageSize = wxSize{ 1280, 720 };
        }

    wxString css{ options.m_css };
    // keep the CSS from ending the <style> element
    css.Replace(L"</", L"<\\/");

    wxString initialMode{ L"auto" };
    wxString rootStyle;
    if (options.m_colorMode == HtmlDashboardOptions::ColorMode::Light)
        {
        initialMode = L"light";
        rootStyle = L" style=\"color-scheme: light\"";
        }
    else if (options.m_colorMode == HtmlDashboardOptions::ColorMode::Dark)
        {
        initialMode = L"dark";
        rootStyle = L" style=\"color-scheme: dark\"";
        }

    const wxString initialView{ HtmlDashboardOptions::ViewToString(options.m_view) };

    // user-facing text used by the script
    const std::vector<std::pair<wxString, wxString>> scriptStrings{
        { L"atlas", _(L"Atlas") },           { L"story", _(L"Story") },
        { L"page", _(L"Page {0}") },         { L"pageOf", _(L"Page {0} of {1}") },
        { L"goTo", _(L"Go to {0}") },        { L"pagesShown", _(L"{0} of {1} pages shown") },
        { L"noPages", _(L"No pages shown") }
    };
    wxString stringsObject{ L"{" };
    for (const auto& [key, value] : scriptStrings)
        {
        stringsObject += key + L": " + jsString(value) + L", ";
        }
    stringsObject += L"}";

    // distinct layers in order of first appearance (an empty layer is always shown)
    std::vector<wxString> distinctLayers;
    for (const auto* canvas : canvases)
        {
        if (canvas != nullptr && !canvas->GetLayer().empty() &&
            std::find(distinctLayers.cbegin(), distinctLayers.cend(), canvas->GetLayer()) ==
                distinctLayers.cend())
            {
            distinctLayers.push_back(canvas->GetLayer());
            }
        }

    wxString layersArray{ L"[" };
    for (const auto& layer : distinctLayers)
        {
        layersArray += jsString(layer) + L", ";
        }
    layersArray += L"]";

    wxString script{ GetDashboardScriptState() + GetDashboardScriptPages() +
                     GetDashboardScriptInk() + GetDashboardScriptNavigation() +
                     GetDashboardScriptCounters() + GetDashboardScriptMotion() +
                     GetDashboardScriptEvents() };
    script.Replace(L"{{TOGGLE}}", options.m_includeColorModeToggle ? L"true" : L"false");
    script.Replace(L"{{COUNTUP}}", options.m_countUpNumbers ? L"true" : L"false");
    script.Replace(L"{{MODE}}", initialMode);
    script.Replace(L"{{VIEW}}", initialView);
    // user-derived text goes in last so that it is never scanned for placeholders
    script.Replace(L"{{STRINGS}}", stringsObject);
    script.Replace(L"{{LAYERS}}", layersArray);

    wxString html;
    html += L"<!DOCTYPE html>\n";
    html += wxString::Format(L"<html lang=\"en\" data-view=\"%s\"%s>\n", initialView, rootStyle);
    html += L"<head>\n"
            "<meta charset=\"utf-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "<meta name=\"color-scheme\" content=\"light dark\">\n"
            "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; "
            "script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data:; "
            "base-uri 'none'; form-action 'none'\">\n";
    if (wxTheApp != nullptr && !wxTheApp->GetAppDisplayName().empty())
        {
        html += wxString::Format(L"<meta name=\"generator\" content=\"%s\">\n",
                                 escapeAttr(wxTheApp->GetAppDisplayName()));
        }
    html += wxString::Format(L"<title>%s</title>\n", escapeText(title));
    html += L"<style>\n" + css + L"\n</style>\n";
    html += L"<script>" + script + L"</script>\n";
    html += L"</head>\n<body>\n";

    html += L"<header class=\"dash-toolbar no-print\">\n";
    html += wxString::Format(L"<h1 class=\"dash-title\">%s</h1>\n", escapeText(title));
    html += L"<div class=\"dash-controls\">\n";
    html += wxString::Format(
        L"<div class=\"dash-pager\" role=\"group\" aria-label=\"%s\">\n"
        "<button type=\"button\" id=\"dash-prev\" aria-label=\"%s\">&lsaquo;</button>\n"
        "<span id=\"dash-pager-label\" class=\"dash-pager-label\"></span>\n"
        "<button type=\"button\" id=\"dash-next\" aria-label=\"%s\">&rsaquo;</button>\n"
        "</div>\n",
        escapeAttr(_(L"Pages")), escapeAttr(_(L"Previous page")), escapeAttr(_(L"Next page")));
    html += wxString::Format(
        L"<div class=\"dash-views\" role=\"group\" aria-label=\"%s\">\n"
        "<button type=\"button\" data-view=\"atlas\" aria-pressed=\"false\">%s</button>\n"
        "<button type=\"button\" data-view=\"story\" aria-pressed=\"false\">%s</button>\n"
        "</div>\n",
        escapeAttr(_(L"Views")), escapeText(_(L"Atlas")), escapeText(_(L"Story")));
    if (!distinctLayers.empty())
        {
        html += wxString::Format(
            L"<div class=\"dash-layers\" role=\"group\" "
            "aria-labelledby=\"dash-layers-label\">\n"
            "<span id=\"dash-layers-label\" class=\"dash-group-label\">%s</span>\n",
            escapeText(_(L"Layers")));
        for (const auto& layer : distinctLayers)
            {
            html += wxString::Format(
                L"<button type=\"button\" data-layer=\"%s\" aria-pressed=\"true\">%s</button>\n",
                escapeAttr(layer), escapeText(layer));
            }
        html += L"</div>\n";
        }
    if (options.m_includeColorModeToggle)
        {
        html += wxString::Format(
            L"<div class=\"dash-modes\" role=\"group\" aria-label=\"%s\">\n"
            "<button type=\"button\" data-mode=\"auto\" aria-pressed=\"false\">%s</button>\n"
            "<button type=\"button\" data-mode=\"light\" aria-pressed=\"false\">%s</button>\n"
            "<button type=\"button\" data-mode=\"dark\" aria-pressed=\"false\">%s</button>\n"
            "</div>\n",
            escapeAttr(_(L"Color mode")), escapeText(_(L"Auto")), escapeText(_(L"Light")),
            escapeText(_(L"Dark")));
        }
    html += L"</div>\n<div class=\"dash-progress\" aria-hidden=\"true\"></div>\n</header>\n";

    html += wxString::Format(
        L"<nav id=\"dash-atlas\" class=\"dash-atlas no-print\" aria-label=\"%s\"></nav>\n"
        "<nav id=\"dash-rail\" class=\"dash-rail no-print\" aria-label=\"%s\"></nav>\n"
        "<div id=\"dash-status\" class=\"visually-hidden\" role=\"status\" "
        "aria-live=\"polite\"></div>\n",
        escapeAttr(_(L"Pages")), escapeAttr(_(L"Pages")));

    html += wxString::Format(L"<main class=\"dash-pages\" style=\"--page-w:%d;--page-h:%d\">\n",
                             pageSize.GetWidth(), pageSize.GetHeight());
    size_t pageIndex{ 0 };
    for (size_t canvasIndex = 0; canvasIndex < canvases.size(); ++canvasIndex)
        {
        auto* canvas = canvases[canvasIndex];
        if (canvas == nullptr)
            {
            continue;
            }
        wxString pageTitle{ (canvasIndex < options.m_pageTitles.size() &&
                             !options.m_pageTitles[canvasIndex].empty()) ?
                                options.m_pageTitles[canvasIndex] :
                                canvas->GetLabel() };
        if (pageTitle.empty())
            {
            pageTitle = wxString::Format(_(L"Page %zu"), pageIndex + 1);
            }
        const wxString pageSvg{ SVGReportPrintout::RenderCanvasToSvg(canvas, pageSize) };
        html += wxString::Format(
            L"<section class=\"page\" id=\"page-%zu\" data-index=\"%zu\" data-layer=\"%s\" "
            "aria-label=\"%s\" tabindex=\"-1\">\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"page-svg\" "
            "viewBox=\"0 0 %d %d\" preserveAspectRatio=\"xMidYMid meet\">\n"
            "<g id=\"page-content-%zu\">\n",
            pageIndex, pageIndex, escapeAttr(canvas->GetLayer()), escapeAttr(pageTitle),
            pageSize.GetWidth(), pageSize.GetHeight(), pageIndex);
        html += pageSvg;
        html += wxString::Format(L"\n</g>\n</svg>\n<h2 class=\"page-title\">%s</h2>\n</section>\n",
                                 escapeText(pageTitle));
        ++pageIndex;
        }
    html += L"</main>\n</body>\n</html>\n";

    wxFile outFile(options.m_filePath, wxFile::write);
    if (outFile.IsOpened())
        {
        outFile.Write(html, wxConvUTF8);
        }
    else
        {
        wxMessageBox(
            wxString::Format(_(L"Failed to save HTML dashboard to \"%s\"."), options.m_filePath),
            _(L"Export Error"), wxOK | wxICON_ERROR);
        }
    }
