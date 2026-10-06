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
#include <wx/base64.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptState()
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
    document.querySelectorAll('#dash-theme-menu button').forEach(function(item) {
      item.setAttribute('aria-checked', item.dataset.mode === colorMode ? 'true' : 'false');
    });
    const themeIcon = document.querySelector('#dash-theme .dash-theme-icon');
    const activeIcon = document.querySelector('#dash-theme-menu [data-mode="' + colorMode +
      '"] .dash-theme-icon');
    if (themeIcon && activeIcon) themeIcon.textContent = activeIcon.textContent;
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
  function markReady() {
    root.classList.remove('is-loading');
  }
  root.style.colorScheme = schemeFor(colorMode);
  if (!reduceMotion.matches) root.classList.add('motion-ready');
  root.classList.add('is-loading');
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptPages()
    {
    return LR"JS(
  function collectPages() {
    pages = Array.from(document.querySelectorAll('.page')).map(function(el, i) {
      const svgs = el.querySelectorAll('.page-svg');
      const page = {
        el: el,
        svg: svgs[0] || null,
        layer: el.getAttribute('data-layer') || '',
        title: el.getAttribute('aria-label') || format(strings.page, i + 1)
      };
      // a second SVG is the same page in the other orientation
      if (svgs.length > 1) {
        page.alt = { el: el, svg: svgs[1], layer: page.layer, title: page.title };
      }
      return page;
    });
  }
  function visibleSvg(page) {
    if (page.alt && page.svg && getComputedStyle(page.svg).visibility === 'hidden') {
      return page.alt.svg;
    }
    return page.svg;
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
    document.querySelectorAll('.dash-layers input[data-layer]').forEach(function(box) {
      box.checked = activeLayers.has(box.dataset.layer);
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
    if (wanted === 'gallery' || wanted === 'story') view = wanted;
    const index = parseInt(params.get('page'), 10);
    if (!isNaN(index)) current = clampIndex(index);
  }
  function writeHash() {
    try { history.replaceState(null, '', '#view=' + view + '&page=' + current); } catch (e) {}
  }
  function measureChrome() {
    const toolbar = document.querySelector('.dash-toolbar');
    // a floating toolbar (presentation mode) takes no room from the page
    if (toolbar) {
      root.style.setProperty('--toolbar-height',
        (root.classList.contains('is-presenting') ? 0 : toolbar.offsetHeight) + 'px');
    }
    const strip = document.getElementById('dash-gallery');
    root.style.setProperty('--strip-height',
                           view === 'gallery' && strip ? strip.offsetHeight + 'px' : '0px');
  }
  function buildGallery() {
    const strip = document.getElementById('dash-gallery');
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
  function updateProgress() {
    if (view !== 'story') return;
    const span = document.documentElement.scrollHeight - window.innerHeight;
    root.style.setProperty('--progress', span > 0 ? String(Math.min(1, window.scrollY / span)) : '0');
  }
  function replayShown() {
    pages.forEach(function(page) {
      const shown = page.el.getClientRects().length > 0;
      if (shown && page.shown === false && page.revealed) {
        startCounters(page);
        if (page.alt) startCounters(page.alt);
      }
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
    updateFilters();
    updateProgress();
    measureChrome();
    writeHash();
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptFilters()
    {
    return LR"JS(
  const hiddenFilters = new Set();
  function applyFilters() {
    document.querySelectorAll('.page-svg [data-filter]').forEach(function(el) {
      const key = scopeIdFor(el) + '|' + el.getAttribute('data-filter');
      el.classList.toggle('is-filtered-out', hiddenFilters.has(key));
    });
    // legend rows go away with their data, leaving a gap
    document.querySelectorAll('.page-svg [data-legend-filter]').forEach(function(el) {
      const key = scopeIdFor(el) + '|' + el.getAttribute('data-legend-filter');
      el.classList.toggle('is-filtered-out', hiddenFilters.has(key));
    });
    // nested items (e.g., an inner pie ring) also go away with their parent
    document.querySelectorAll('.page-svg [data-filter-parent], .page-svg [data-legend-parent]')
      .forEach(function(el) {
        const parent = el.getAttribute('data-filter-parent') ||
                       el.getAttribute('data-legend-parent');
        if (hiddenFilters.has(scopeIdFor(el) + '|' + parent)) el.classList.add('is-filtered-out');
      });
    // a base layer goes away once every group in its region is filtered out
    document.querySelectorAll('.page-svg [data-filter-all]').forEach(function(el) {
      const scope = scopeIdFor(el);
      const allHidden = el.getAttribute('data-filter-all')
        .split(String.fromCharCode(0x2028)).every(function(label) {
          return hiddenFilters.has(scope + '|' + label);
        });
      el.classList.toggle('is-filtered-out', allHidden);
    });
    document.querySelectorAll('.dash-filters input[data-filter-key]').forEach(function(box) {
      box.checked = !hiddenFilters.has(box.dataset.filterKey);
    });
  }
  function toggleFilter(key) {
    if (hiddenFilters.has(key)) hiddenFilters.delete(key);
    else hiddenFilters.add(key);
    applyFilters();
  }
  function buildFilters() {
    const menu = document.getElementById('dash-filters-menu');
    if (!menu) return;
    pages.forEach(function(page, pageIndex) {
      if (!page.svg) return;
      // a key is either a plain label or "ring", U+2029, and "label" (e.g., for a two-ring pie)
      const fieldSeparator = String.fromCharCode(0x2029);
      const sections = new Map();
      const chartIds = [];
      page.svg.querySelectorAll('[data-filter]').forEach(function(el) {
        const chartId = scopeIdFor(el);
        const key = el.getAttribute('data-filter');
        const splitAt = key.indexOf(fieldSeparator);
        const ring = splitAt < 0 ? '' : key.substring(0, splitAt);
        const text = splitAt < 0 ? key : key.substring(splitAt + 1);
        const sectionKey = chartId + fieldSeparator + ring;
        if (chartIds.indexOf(chartId) < 0) chartIds.push(chartId);
        if (!sections.has(sectionKey)) {
          sections.set(sectionKey, {
            chartId: chartId,
            ring: ring,
            title: el.getAttribute('data-filter-title') || '',
            labels: []
          });
        }
        const labels = sections.get(sectionKey).labels;
        if (!labels.some(function(entry) { return entry.key === key; })) {
          labels.push({ key: key, text: text });
        }
      });
      sections.forEach(function(section) {
        let title = section.title;
        if (!title) {
          title = format(strings.chart, chartIds.indexOf(section.chartId) + 1);
        }
        if (section.ring) title += ': ' + section.ring;
        const group = document.createElement('div');
        group.className = 'dash-filters-group';
        group.dataset.page = String(pageIndex);
        group.setAttribute('role', 'group');
        group.setAttribute('aria-label', title);
        const heading = document.createElement('div');
        heading.className = 'dash-filters-heading';
        heading.textContent = title;
        group.appendChild(heading);
        section.labels.forEach(function(entry) {
          const row = document.createElement('label');
          const box = document.createElement('input');
          box.type = 'checkbox';
          box.checked = true;
          box.dataset.filterKey = section.chartId + '|' + entry.key;
          box.addEventListener('change', function() { toggleFilter(box.dataset.filterKey); });
          row.appendChild(box);
          row.appendChild(document.createTextNode(entry.text));
          group.appendChild(row);
        });
        menu.appendChild(group);
      });
    });
    updateFilters();
  }
  function updateFilters() {
    const menu = document.getElementById('dash-filters-menu');
    const wrapper = document.querySelector('.dash-filters');
    if (!menu || !wrapper) return;
    let shown = 0;
    menu.querySelectorAll('.dash-filters-group').forEach(function(group) {
      const isCurrent = group.dataset.page === String(current);
      group.hidden = !isCurrent;
      if (isCurrent) ++shown;
    });
    wrapper.hidden = shown === 0;
    if (shown === 0) {
      menu.hidden = true;
      const btn = wrapper.querySelector('.dash-filters-btn');
      if (btn) btn.setAttribute('aria-expanded', 'false');
    }
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptInk()
    {
    return LR"JS(
  function paintKind(el) {
    const holder = el.closest('[fill]');
    if (!holder) return 'none';
    const fill = holder.getAttribute('fill').toUpperCase();
    if (fill === 'NONE') return 'none';
    const opacity = parseFloat(holder.getAttribute('fill-opacity'));
    if (!isNaN(opacity) && opacity < 0.5) return 'none';
    if (el.closest('.ink-keep')) return 'color';
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
    const keep = [];
    const nodes = page.svg.querySelectorAll('rect, path, polygon, ellipse, circle, text');
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
        if (shape.kind === 'color') keep.push(el);
        return;
      }
    });
    // classes are added after all measuring so that layout is not forced between texts
    keep.forEach(function(el) { el.classList.add('ink-keep'); });
  }
  function assignInk() {
    const hidden = pages.filter(function(page) { return page.el.getClientRects().length === 0; });
    hidden.forEach(function(page) { page.el.classList.add('ink-measure'); });
    pages.forEach(function(page) {
      try { tagInk(page); } catch (e) {}
      try { if (page.alt) tagInk(page.alt); } catch (e) {}
    });
    hidden.forEach(function(page) { page.el.classList.remove('ink-measure'); });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptNavigation()
    {
    return LR"JS(
  function revealCurrent(behavior) {
    const page = pages[current];
    if (!page) return;
    if (view === 'gallery') {
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
    if (view === 'gallery') {
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptCounters()
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptMotion()
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
  function isNeutral(key) {
    // grayscale-ish (low saturation): axis lines, gridlines, outlines, not a data color
    const c = key.split(',').map(Number);
    return (Math.max(c[0], c[1], c[2]) - Math.min(c[0], c[1], c[2])) < 12;
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
  function scopeIdFor(el) {
    const scoped = el.closest('[data-chart-id]');
    return scoped ? scoped.getAttribute('data-chart-id') : '';
  }
  function getScope(page, scopeId) {
    if (!page.scopes.has(scopeId)) {
      page.scopes.set(scopeId, { spot: new Map(), keys: new WeakMap() });
    }
    return page.scopes.get(scopeId);
  }
  function registerSpot(page, el, key) {
    if (!key || isNearWhite(key)) return;
    el.classList.add('mk-spot');
    const scope = getScope(page, scopeIdFor(el));
    scope.keys.set(el, key);
    if (!scope.spot.has(key)) scope.spot.set(key, []);
    scope.spot.get(key).push(el);
  }
  function prepareMarks(page) {
    if (page.prepared) return;
    page.prepared = true;
    const svg = page.svg;
    if (!svg) return;
    let fadeIndex = 0;
    const shapes = svg.querySelectorAll(shapeSelector);
    if (shapes.length <= maxAnimatedMarks) {
      const viewBox = svg.viewBox.baseVal;
      const limitArea = viewBox.width * viewBox.height * 0.2;
      page.scopes = new Map();
      // all reads come before any writes so that layout is computed once
      const marks = [];
      shapes.forEach(function(el) {
        const style = getComputedStyle(el);
        const kind = classifyMark(el, style, limitArea);
        if (!kind) return;
        let length = 0;
        if (kind === 'draw') {
          try { length = el.getTotalLength(); } catch (e) {}
          if (!(length > 1)) return;
        }
        let key = '';
        if (kind === 'pop' || kind === 'grow') {
          key = rgbKey(style.fill);
        } else if (kind === 'draw') {
          const strokeKey = rgbKey(style.stroke);
          if (strokeKey && !isNeutral(strokeKey)) key = strokeKey;
        }
        marks.push({ el: el, kind: kind, length: length, key: key });
      });
      let index = 0;
      marks.forEach(function(mark) {
        if (mark.kind === 'draw') mark.el.style.setProperty('--len', String(mark.length));
        mark.el.classList.add('mk', 'mk-' + mark.kind);
        mark.el.style.setProperty('--i', String(mark.kind === 'fade' ? fadeIndex++ : index++));
        registerSpot(page, mark.el, mark.key);
      });
    }
    svg.querySelectorAll('text').forEach(function(el) {
      el.classList.add('mk', 'mk-fade');
      el.style.setProperty('--i', String(fadeIndex++));
    });
    if (countUp) page.counters = collectCounters(svg);
  }
  function bindSpot(page) {
    if (page.spotBound || !page.scopes || !page.scopes.size) return;
    page.spotBound = true;
    const svg = page.svg;
    let currentScope = null;
    let currentKey = '';
    function clear() {
      if (currentScope && currentKey) {
        currentScope.spot.get(currentKey).forEach(function(el) { el.classList.remove('is-spot'); });
      }
      currentScope = null;
      currentKey = '';
      svg.classList.remove('is-spotting');
    }
    function setKey(scope, key) {
      if (scope === currentScope && key === currentKey) return;
      clear();
      if (!scope || !key || scope.spot.size < 2) return;
      currentScope = scope;
      currentKey = key;
      scope.spot.get(key).forEach(function(el) { el.classList.add('is-spot'); });
      svg.classList.add('is-spotting');
    }
    svg.addEventListener('mouseover', function(e) {
      const scope = page.scopes.get(scopeIdFor(e.target));
      setKey(scope, scope ? scope.keys.get(e.target) : undefined);
    });
    svg.addEventListener('mouseleave', clear);
  }
  const slowPageElements = 400;
  let pendingSlow = 0;

  function scheduleReveal(page) {
    const svg = page.svg;
    const size = svg ? svg.querySelectorAll(shapeSelector + ', text').length : 0;
    if (size <= slowPageElements) {
      revealPage(page);
      if (!pendingSlow) markReady();
      return;
    }
    ++pendingSlow;
    root.classList.add('is-loading');
    window.requestAnimationFrame(function() {
      window.setTimeout(function() {
        revealPage(page);
        if (--pendingSlow === 0) markReady();
      }, 0);
    });
  }
  function revealPage(page) {
    if (page.alt) revealPage(page.alt);
    prepareMarks(page);
    bindSpot(page);
    bindZoom(page);
    startCounters(page);
    const svg = page.svg;
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
    const svg = page.svg;
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
        scheduleReveal(page);
      });
    }, { threshold: 0.2 });
    pages.forEach(function(page) { observer.observe(page.el); });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptZoom()
    {
    return LR"JS(
  const maxZoomScale = 8;
  function bindZoom(page) {
    if (page.zoomBound) return;
    page.zoomBound = true;
    const svg = page.svg;
    if (!svg) return;
    const base = svg.viewBox.baseVal;
    const baseBox = { x: base.x, y: base.y, width: base.width, height: base.height };
    let scale = 1;
    let vbx = baseBox.x, vby = baseBox.y, vbw = baseBox.width, vbh = baseBox.height;
    function apply() {
      svg.setAttribute('viewBox', vbx + ' ' + vby + ' ' + vbw + ' ' + vbh);
      svg.classList.toggle('is-zoomed', scale > 1);
    }
    function clamp() {
      vbx = Math.min(Math.max(vbx, baseBox.x), baseBox.x + baseBox.width - vbw);
      vby = Math.min(Math.max(vby, baseBox.y), baseBox.y + baseBox.height - vbh);
    }
    function reset() {
      scale = 1;
      vbx = baseBox.x;
      vby = baseBox.y;
      vbw = baseBox.width;
      vbh = baseBox.height;
      apply();
    }
    function pointToSvg(clientX, clientY) {
      const rect = svg.getBoundingClientRect();
      return {
        x: vbx + (clientX - rect.left) / rect.width * vbw,
        y: vby + (clientY - rect.top) / rect.height * vbh
      };
    }
    function zoomAt(clientX, clientY, factor) {
      const before = pointToSvg(clientX, clientY);
      scale = Math.min(maxZoomScale, Math.max(1, scale * factor));
      if (scale === 1) {
        reset();
        return;
      }
      vbw = baseBox.width / scale;
      vbh = baseBox.height / scale;
      clamp();
      const after = pointToSvg(clientX, clientY);
      vbx += before.x - after.x;
      vby += before.y - after.y;
      clamp();
      apply();
    }
    svg.addEventListener('wheel', function(e) {
      if (!e.ctrlKey && !e.metaKey) return;
      e.preventDefault();
      zoomAt(e.clientX, e.clientY, e.deltaY < 0 ? 1.2 : 1 / 1.2);
    }, { passive: false });
    let dragging = false;
    let lastX = 0;
    let lastY = 0;
    svg.addEventListener('pointerdown', function(e) {
      if (scale <= 1 || e.button !== 0) return;
      dragging = true;
      lastX = e.clientX;
      lastY = e.clientY;
      svg.setPointerCapture(e.pointerId);
      svg.classList.add('is-panning');
      e.preventDefault();
    });
    svg.addEventListener('pointermove', function(e) {
      if (!dragging) return;
      const rect = svg.getBoundingClientRect();
      vbx -= (e.clientX - lastX) / rect.width * vbw;
      vby -= (e.clientY - lastY) / rect.height * vbh;
      lastX = e.clientX;
      lastY = e.clientY;
      clamp();
      apply();
    });
    function endDrag() {
      if (!dragging) return;
      dragging = false;
      svg.classList.remove('is-panning');
    }
    svg.addEventListener('pointerup', endDrag);
    svg.addEventListener('pointercancel', endDrag);
    svg.addEventListener('dblclick', function(e) {
      e.preventDefault();
      reset();
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptTooltips()
    {
    return LR"JS(
  function bindTooltips() {
    const tip = document.getElementById('dash-tooltip');
    if (!tip) return;
    let target = null;
    function place(e) {
      const margin = 14;
      let x = e.clientX + margin;
      let y = e.clientY + margin;
      const rect = tip.getBoundingClientRect();
      if (x + rect.width > window.innerWidth) x = e.clientX - rect.width - margin;
      if (y + rect.height > window.innerHeight) y = e.clientY - rect.height - margin;
      tip.style.left = Math.max(0, x) + 'px';
      tip.style.top = Math.max(0, y) + 'px';
    }
    // a grouped region lists only its shown groups once any of them is filtered out
    function tipText(hit) {
      const lineBreak = String.fromCharCode(0x2028);
      const groups = hit.getAttribute('data-tip-groups');
      if (groups) {
        const scope = scopeIdFor(hit);
        const entries = groups.split(lineBreak).map(function(entry) {
          return entry.split(String.fromCharCode(0x2029));
        });
        const shown = entries.filter(function(entry) {
          return !hiddenFilters.has(scope + '|' + entry[0]);
        });
        if (shown.length !== entries.length) {
          const head = hit.getAttribute('data-tip-head');
          const note = hit.getAttribute('data-tip-note');
          return (head ? [head] : []).concat(note ? [note] : [], shown.map(function(entry) {
            return entry[1];
          })).join('\n');
        }
      }
      return hit.getAttribute('aria-label').split(lineBreak).join('\n');
    }
    document.addEventListener('mouseover', function(e) {
      const hit = e.target.closest('.page-svg [role="img"][aria-label]');
      if (!hit || hit === target) return;
      const text = tipText(hit);
      if (!text) return;
      target = hit;
      const head = hit.getAttribute('data-tip-head');
      tip.textContent = '';
      if (head && text.startsWith(head)) {
        const rest = text.slice(head.length);
        const hasLines = rest.charAt(0) === '\n';
        const title = document.createElement(hasLines ? 'div' : 'strong');
        title.textContent = head;
        if (hasLines) title.className = 'dash-tooltip-head';
        tip.appendChild(title);
        tip.appendChild(document.createTextNode(hasLines ? rest.replace(/^\n+/, '') : rest));
      } else {
        tip.textContent = text;
      }
      tip.classList.add('is-visible');
      place(e);
    });
    document.addEventListener('mousemove', function(e) {
      if (target) place(e);
    });
    document.addEventListener('mouseout', function(e) {
      if (!target) return;
      if (e.relatedTarget && target.contains(e.relatedTarget)) return;
      target = null;
      tip.classList.remove('is-visible');
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptHelp()
    {
    return LR"JS(
  let helpOpen = false;
  function bindHelp() {
    const btn = document.getElementById('dash-help');
    const panel = document.getElementById('dash-help-panel');
    if (!btn || !panel) return;
    const closeBtn = document.getElementById('dash-help-close');
    function positionPanel() {
      const rect = btn.getBoundingClientRect();
      const panelRect = panel.getBoundingClientRect();
      const left = Math.min(Math.max(8, rect.right - panelRect.width),
        window.innerWidth - panelRect.width - 8);
      panel.style.left = left + 'px';
      panel.style.top = (rect.bottom + 8) + 'px';
    }
    function onOutsidePointerDown(e) {
      if (panel.contains(e.target) || e.target === btn) return;
      closeHelp(false);
    }
    function onHelpKeyDown(e) {
      if (e.key === 'Escape') {
        e.preventDefault();
        closeHelp(true);
      }
    }
    function openHelp() {
      helpOpen = true;
      panel.hidden = false;
      positionPanel();
      window.requestAnimationFrame(function() { panel.classList.add('is-open'); });
      btn.setAttribute('aria-expanded', 'true');
      document.addEventListener('keydown', onHelpKeyDown);
      document.addEventListener('pointerdown', onOutsidePointerDown, true);
      window.addEventListener('resize', positionPanel);
      if (closeBtn) closeBtn.focus();
    }
    function closeHelp(returnFocus) {
      helpOpen = false;
      panel.classList.remove('is-open');
      btn.setAttribute('aria-expanded', 'false');
      document.removeEventListener('keydown', onHelpKeyDown);
      document.removeEventListener('pointerdown', onOutsidePointerDown, true);
      window.removeEventListener('resize', positionPanel);
      window.setTimeout(function() { panel.hidden = true; }, 150);
      if (returnFocus) btn.focus();
    }
    btn.addEventListener('click', function() {
      if (panel.hidden) openHelp(); else closeHelp(true);
    });
    if (closeBtn) {
      closeBtn.addEventListener('click', function() { closeHelp(true); });
    }
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptSave()
    {
    return LR"JS(
  let saveOpen = false;
  function pageContentSize(svg) {
    const style = svg ? getComputedStyle(svg) : null;
    const width = style ?
      parseFloat(style.getPropertyValue('--svg-w') || style.getPropertyValue('--page-w')) : 0;
    const height = style ?
      parseFloat(style.getPropertyValue('--svg-h') || style.getPropertyValue('--page-h')) : 0;
    return { width: width || 0, height: height || 0 };
  }
  function slugify(text) {
    const slug = (text || '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '');
    return slug || 'page';
  }
  function downloadBlob(blob, filename) {
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    link.remove();
    window.setTimeout(function() { URL.revokeObjectURL(url); }, 1000);
  }
  // colors painted into pages are remapped by the stylesheet for light/dark mode, so the
  // resolved colors are baked into the exported copy to keep it correct outside the dashboard
  function clonePageSvg(page, size) {
    const svg = visibleSvg(page);
    if (!svg) return null;
    const clone = svg.cloneNode(true);
    clone.removeAttribute('class');
    clone.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    clone.setAttribute('viewBox', '0 0 ' + size.width + ' ' + size.height);
    clone.setAttribute('width', String(size.width));
    clone.setAttribute('height', String(size.height));
    const liveNodes = svg.querySelectorAll('[fill], [stroke]');
    const cloneNodes = clone.querySelectorAll('[fill], [stroke]');
    liveNodes.forEach(function(el, i) {
      const target = cloneNodes[i];
      if (!target) return;
      const computed = getComputedStyle(el);
      if (el.hasAttribute('fill')) target.setAttribute('fill', computed.fill);
      if (el.hasAttribute('stroke')) target.setAttribute('stroke', computed.stroke);
    });
    // the stylesheet that hides filtered items isn't part of the exported copy
    clone.querySelectorAll('.is-filtered-out').forEach(function(el) { el.remove(); });
    const background = getComputedStyle(svg).backgroundColor;
    if (background) {
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('x', '0');
      rect.setAttribute('y', '0');
      rect.setAttribute('width', String(size.width));
      rect.setAttribute('height', String(size.height));
      rect.setAttribute('fill', background);
      clone.insertBefore(rect, clone.firstChild);
    }
    return clone;
  }
  function savePageAsSvg(page) {
    const clone = clonePageSvg(page, pageContentSize(visibleSvg(page)));
    if (!clone) return;
    const xml = '<?xml version="1.0" encoding="UTF-8"?>\n' +
      new XMLSerializer().serializeToString(clone);
    downloadBlob(new Blob([xml], { type: 'image/svg+xml' }), slugify(page.title) + '.svg');
  }
  function savePageAsPng(page) {
    const size = pageContentSize(visibleSvg(page));
    const clone = clonePageSvg(page, size);
    if (!clone) return;
    const xml = new XMLSerializer().serializeToString(clone);
    const scale = window.devicePixelRatio || 1;
    const img = new Image();
    img.onload = function() {
      const canvas = document.createElement('canvas');
      canvas.width = Math.round(size.width * scale);
      canvas.height = Math.round(size.height * scale);
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
      canvas.toBlob(function(blob) {
        if (blob) downloadBlob(blob, slugify(page.title) + '.png');
      }, 'image/png');
    };
    img.src = 'data:image/svg+xml;charset=utf-8;base64,' +
      btoa(unescape(encodeURIComponent(xml)));
  }
  function bindSave() {
    const btn = document.getElementById('dash-save');
    const menu = document.getElementById('dash-save-menu');
    if (!btn || !menu) return;
    function positionMenu() {
      const rect = btn.getBoundingClientRect();
      const menuRect = menu.getBoundingClientRect();
      const left = Math.min(Math.max(8, rect.right - menuRect.width),
        window.innerWidth - menuRect.width - 8);
      menu.style.left = left + 'px';
      menu.style.top = (rect.bottom + 8) + 'px';
    }
    function onOutsidePointerDown(e) {
      if (menu.contains(e.target) || e.target === btn) return;
      closeMenu(false);
    }
    function onMenuKeyDown(e) {
      if (e.key === 'Escape') {
        e.preventDefault();
        closeMenu(true);
      } else if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
        const items = Array.prototype.slice.call(menu.querySelectorAll('button'));
        if (!items.length) return;
        const at = items.indexOf(document.activeElement);
        const step = e.key === 'ArrowDown' ? 1 : items.length - 1;
        e.preventDefault();
        items[at < 0 ? (step === 1 ? 0 : items.length - 1) : (at + step) % items.length].focus();
      }
    }
    function openMenu() {
      saveOpen = true;
      menu.hidden = false;
      positionMenu();
      window.requestAnimationFrame(function() { menu.classList.add('is-open'); });
      btn.setAttribute('aria-expanded', 'true');
      document.addEventListener('keydown', onMenuKeyDown);
      document.addEventListener('pointerdown', onOutsidePointerDown, true);
      window.addEventListener('resize', positionMenu);
      const first = menu.querySelector('button');
      if (first) first.focus();
    }
    function closeMenu(returnFocus) {
      saveOpen = false;
      menu.classList.remove('is-open');
      btn.setAttribute('aria-expanded', 'false');
      document.removeEventListener('keydown', onMenuKeyDown);
      document.removeEventListener('pointerdown', onOutsidePointerDown, true);
      window.removeEventListener('resize', positionMenu);
      window.setTimeout(function() { menu.hidden = true; }, 150);
      if (returnFocus) btn.focus();
    }
    btn.addEventListener('click', function() {
      if (menu.hidden) openMenu(); else closeMenu(true);
    });
    menu.querySelectorAll('button[data-format]').forEach(function(item) {
      item.addEventListener('click', function() {
        closeMenu(true);
        const page = pages[current];
        if (!page) return;
        if (item.dataset.format === 'svg') savePageAsSvg(page); else savePageAsPng(page);
      });
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptPrint()
    {
    return LR"JS(
  function clearPrintMarks() {
    root.classList.remove('is-printing');
    document.querySelectorAll('.print-page, .print-svg').forEach(function(el) {
      el.classList.remove('print-page', 'print-svg');
    });
  }
  function printPage(page) {
    const svg = visibleSvg(page);
    if (!svg) return;
    clearPrintMarks();
    page.el.classList.add('print-page');
    svg.classList.add('print-svg');
    root.classList.add('is-printing');
    window.print();
  }
  function bindPrint() {
    const btn = document.getElementById('dash-print');
    if (!btn) return;
    window.addEventListener('afterprint', clearPrintMarks);
    btn.addEventListener('click', function() {
      const page = pages[current];
      if (page) printPage(page);
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptPresent()
    {
    return LR"JS(
  function togglePresenting() {
    if (!document.fullscreenEnabled) return;
    if (document.fullscreenElement) {
      document.exitFullscreen();
    } else {
      root.requestFullscreen().catch(function() {});
    }
  }
  function onPresentPointerMove(e) {
    const toolbar = document.querySelector('.dash-toolbar');
    if (!toolbar) return;
    if (e.clientY <= 8) {
      root.classList.add('show-toolbar');
    } else if (e.clientY > toolbar.offsetHeight + 24 &&
               !toolbar.querySelector('[aria-expanded="true"]') &&
               !toolbar.matches(':focus-within')) {
      root.classList.remove('show-toolbar');
    }
  }
  function onFullscreenChange() {
    const on = !!document.fullscreenElement;
    root.classList.toggle('is-presenting', on);
    root.classList.remove('show-toolbar');
    if (on) document.addEventListener('mousemove', onPresentPointerMove);
    else document.removeEventListener('mousemove', onPresentPointerMove);
    const btn = document.getElementById('dash-full');
    if (btn) btn.setAttribute('aria-pressed', on ? 'true' : 'false');
    // the toolbar and strip are gone (or back), so the page is re-fitted and re-aligned
    window.requestAnimationFrame(function() {
      measureChrome();
      revealCurrent('auto');
    });
  }
  function bindPresent() {
    const btn = document.getElementById('dash-full');
    if (!btn || !document.fullscreenEnabled) return;
    btn.hidden = false;
    btn.addEventListener('click', togglePresenting);
    document.addEventListener('fullscreenchange', onFullscreenChange);
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptThemeMenu()
    {
    return LR"JS(
  let themeOpen = false;
  let themeBtn = null;
  let themeMenu = null;
  let themeItems = [];
  function positionThemeMenu() {
    const rect = themeBtn.getBoundingClientRect();
    const menuRect = themeMenu.getBoundingClientRect();
    const left = Math.min(Math.max(8, rect.right - menuRect.width),
      window.innerWidth - menuRect.width - 8);
    themeMenu.style.left = left + 'px';
    themeMenu.style.top = (rect.bottom + 8) + 'px';
  }
  function focusThemeItem(index) {
    themeItems[(index + themeItems.length) % themeItems.length].focus();
  }
  function onThemeOutsidePointerDown(e) {
    if (themeMenu.contains(e.target) || themeBtn.contains(e.target)) return;
    closeThemeMenu(false);
  }
  function onThemeKeyDown(e) {
    const at = themeItems.indexOf(document.activeElement);
    if (e.key === 'Escape') {
      e.preventDefault();
      closeThemeMenu(true);
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      focusThemeItem(at < 0 ? 0 : at + 1);
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      focusThemeItem(at < 0 ? themeItems.length - 1 : at - 1);
    } else if (e.key === 'Home') {
      e.preventDefault();
      focusThemeItem(0);
    } else if (e.key === 'End') {
      e.preventDefault();
      focusThemeItem(themeItems.length - 1);
    } else if (e.key === 'Enter' && at >= 0) {
      e.preventDefault();
      themeItems[at].click();
    } else if (e.key === 'Tab') {
      closeThemeMenu(false);
    }
  }
  function openThemeMenu() {
    themeOpen = true;
    themeMenu.hidden = false;
    positionThemeMenu();
    window.requestAnimationFrame(function() { themeMenu.classList.add('is-open'); });
    themeBtn.setAttribute('aria-expanded', 'true');
    document.addEventListener('keydown', onThemeKeyDown);
    document.addEventListener('pointerdown', onThemeOutsidePointerDown, true);
    window.addEventListener('resize', positionThemeMenu);
    const checked = themeItems.findIndex(function(item) {
      return item.getAttribute('aria-checked') === 'true';
    });
    focusThemeItem(checked < 0 ? 0 : checked);
  }
  function closeThemeMenu(returnFocus) {
    themeOpen = false;
    themeMenu.classList.remove('is-open');
    themeBtn.setAttribute('aria-expanded', 'false');
    document.removeEventListener('keydown', onThemeKeyDown);
    document.removeEventListener('pointerdown', onThemeOutsidePointerDown, true);
    window.removeEventListener('resize', positionThemeMenu);
    window.setTimeout(function() { if (!themeOpen) themeMenu.hidden = true; }, 150);
    if (returnFocus) themeBtn.focus();
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptTheme()
    {
    return LR"JS(
  function bindTheme() {
    themeBtn = document.getElementById('dash-theme');
    themeMenu = document.getElementById('dash-theme-menu');
    if (!themeBtn || !themeMenu) return;
    themeItems = Array.prototype.slice.call(themeMenu.querySelectorAll('button'));
    themeBtn.addEventListener('click', function() {
      if (themeOpen) closeThemeMenu(true); else openThemeMenu();
    });
    themeItems.forEach(function(item) {
      item.addEventListener('click', function() {
        setColorMode(item.dataset.mode);
        closeThemeMenu(true);
      });
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptEvents()
    {
    return LR"JS(
  function onKeyDown(e) {
    if (e.defaultPrevented || e.ctrlKey || e.metaKey || e.altKey || !pages.length ||
        helpOpen || saveOpen || themeOpen) {
      return;
    }
    const tag = e.target && e.target.tagName;
    if ((tag === 'INPUT' && e.target.type !== 'checkbox') || tag === 'TEXTAREA' ||
        (e.target && e.target.isContentEditable)) {
      return;
    }
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
    } else if (e.key === 'f' || e.key === 'F') {
      e.preventDefault();
      togglePresenting();
    }
  }
  function bindControls() {
    document.querySelectorAll('.dash-views button').forEach(function(btn) {
      btn.addEventListener('click', function() { setView(btn.dataset.view); });
    });
    document.querySelectorAll('.dash-layers input[data-layer]').forEach(function(box) {
      box.addEventListener('change', function() { toggleLayer(box.dataset.layer); });
    });
    const bindMenu = function(btnSelector, menuId, wrapperSelector) {
      const btn = document.querySelector(btnSelector);
      const menu = document.getElementById(menuId);
      if (!btn || !menu) return;
      const setMenu = function(open) {
        menu.hidden = !open;
        btn.setAttribute('aria-expanded', open ? 'true' : 'false');
      };
      btn.addEventListener('click', function() { setMenu(menu.hidden); });
      const menuBoxes = function() {
        return Array.prototype.filter.call(menu.querySelectorAll('input[type="checkbox"]'),
          function(box) { return box.offsetParent !== null; });
      };
      // up and down arrows move through the checkboxes and Enter toggles the focused one
      const onMenuArrow = function(ev) {
        if (menu.hidden || (ev.key !== 'ArrowDown' && ev.key !== 'ArrowUp')) return;
        const boxes = menuBoxes();
        if (!boxes.length) return;
        const at = boxes.indexOf(document.activeElement);
        let next;
        if (at < 0) {
          next = ev.key === 'ArrowDown' ? 0 : boxes.length - 1;
        } else {
          next = (at + (ev.key === 'ArrowDown' ? 1 : boxes.length - 1)) % boxes.length;
        }
        ev.preventDefault();
        boxes[next].focus();
      };
      btn.addEventListener('keydown', onMenuArrow);
      menu.addEventListener('keydown', function(ev) {
        onMenuArrow(ev);
        if (ev.key === 'Enter' && ev.target.matches('input[type="checkbox"]')) {
          ev.preventDefault();
          ev.target.click();
        }
      });
      document.addEventListener('click', function(ev) {
        if (!menu.hidden && !ev.target.closest(wrapperSelector)) setMenu(false);
      });
      document.addEventListener('keydown', function(ev) {
        if (ev.key === 'Escape' && !menu.hidden) {
          setMenu(false);
          btn.focus();
        }
      });
    };
    bindMenu('.dash-layers-btn', 'dash-layers-menu', '.dash-layers');
    bindMenu('.dash-filters-btn', 'dash-filters-menu', '.dash-filters');
    document.addEventListener('keydown', onKeyDown);
    window.addEventListener('scroll', updateProgress, { passive: true });
    window.addEventListener('resize', measureChrome);
    bindTooltips();
    bindHelp();
    bindSave();
    bindPrint();
    bindPresent();
    bindTheme();
  }
  document.addEventListener('DOMContentLoaded', function() {
    collectPages();
    readHash();
    applyColorMode();
    buildRail();
    buildGallery();
    bindControls();
    applyLayers();
    buildFilters();
    applyFilters();
    renderView();
    revealCurrent('auto');
    observeStory();
    // let the loading indicator paint before the heavy work starts
    window.requestAnimationFrame(function() {
      window.setTimeout(function() {
        assignInk();
        setupMotion();
        if (!pages.length || !root.classList.contains('motion-ready')) markReady();
      }, 0);
    });
  });
})();
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::BuildLogo(HtmlDashboardOptions options)
    {
    wxString logoUri;
    if (!options.m_logoPath.empty() && wxFileName::FileExists(options.m_logoPath))
        {
        const wxString logoExt{ wxFileName{ options.m_logoPath }.GetExt().Lower() };
        wxString mimeType;
        if (logoExt == L"svg")
            {
            mimeType = L"image/svg+xml";
            }
        else if (logoExt == L"png")
            {
            mimeType = L"image/png";
            }
        else if (logoExt == L"jpg" || logoExt == L"jpeg")
            {
            mimeType = L"image/jpeg";
            }
        else if (logoExt == L"gif")
            {
            mimeType = L"image/gif";
            }
        else if (logoExt == L"webp")
            {
            mimeType = L"image/webp";
            }

        wxFile logoFile{ options.m_logoPath };
        if (!mimeType.empty() && logoFile.IsOpened() && logoFile.Length() > 0)
            {
            std::vector<char> logoBytes(static_cast<size_t>(logoFile.Length()));
            if (logoFile.Read(logoBytes.data(), logoBytes.size()) ==
                static_cast<ssize_t>(logoBytes.size()))
                {
                // downscale raster image that are larger than the logo's display size (at 2x)
                if (logoExt != L"svg")
                    {
                    constexpr int MAX_LOGO_WIDTH{ 320 };
                    constexpr int MAX_LOGO_HEIGHT{ 64 };
                    wxMemoryInputStream logoStream(logoBytes.data(), logoBytes.size());
                    wxImage logoImage;
                    if (logoImage.LoadFile(logoStream) && logoImage.IsOk() &&
                        (logoImage.GetWidth() > MAX_LOGO_WIDTH ||
                         logoImage.GetHeight() > MAX_LOGO_HEIGHT))
                        {
                        const double logoScale{ std::min(
                            safe_divide<double>(MAX_LOGO_WIDTH, logoImage.GetWidth()),
                            safe_divide<double>(MAX_LOGO_HEIGHT, logoImage.GetHeight())) };
                        logoImage.Rescale(std::max(1, wxRound(logoImage.GetWidth() * logoScale)),
                                          std::max(1, wxRound(logoImage.GetHeight() * logoScale)),
                                          wxIMAGE_QUALITY_HIGH);
                        wxMemoryOutputStream scaledStream;
                        if (logoImage.SaveFile(scaledStream, wxBITMAP_TYPE_PNG))
                            {
                            logoBytes.resize(scaledStream.GetSize());
                            scaledStream.CopyTo(logoBytes.data(), logoBytes.size());
                            // PNG keeps transparency, regardless of the original format
                            mimeType = L"image/png";
                            }
                        }
                    }
                logoUri = L"data:" + mimeType + L";base64," +
                          wxBase64Encode(logoBytes.data(), logoBytes.size());
                }
            }
        }
    return logoUri;
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::BuildKeyboardTip()
    {
    return wxString::Format(
        L"<div id=\"dash-help-panel\" class=\"dash-help-panel no-print\" role=\"dialog\" "
        "aria-modal=\"false\" aria-labelledby=\"dash-help-title\" hidden>\n"
        "<div class=\"dash-help-header\">\n"
        "<h2 id=\"dash-help-title\">%s</h2>\n"
        "<button type=\"button\" id=\"dash-help-close\" aria-label=\"%s\">&times;</button>\n"
        "</div>\n"
        "<dl class=\"dash-help-list\">\n"
        "<dt><kbd>&larr;</kbd> <kbd>&rarr;</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>Home</kbd> <kbd>End</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>F</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>&uarr;</kbd> <kbd>&darr;</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>Space</kbd> <kbd>Enter</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>Esc</kbd></dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "</dl>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlText(_(L"Keyboard & mouse shortcuts")),
        SVGReportPrintout::EscapeXmlAttr(_(L"Close")),
        SVGReportPrintout::EscapeXmlText(_(L"Go to the previous or next page")),
        SVGReportPrintout::EscapeXmlText(_(L"Jump to the first or last page")),
        SVGReportPrintout::EscapeXmlText(_(L"Enter or leave fullscreen presentation mode")),
        SVGReportPrintout::EscapeXmlText(_(L"Move through the items of a menu (Tab also works)")),
        SVGReportPrintout::EscapeXmlText(_(L"Check or uncheck the selected item in a menu")),
        SVGReportPrintout::EscapeXmlText(_(L"Close the open menu")),
        SVGReportPrintout::EscapeXmlText(_(L"Ctrl") + L"+" + _(L"scroll") + L" / " + _(L"pinch")),
        SVGReportPrintout::EscapeXmlText(_(L"Zoom in or out, centered on the cursor")),
        SVGReportPrintout::EscapeXmlText(_(L"Drag")),
        SVGReportPrintout::EscapeXmlText(_(L"Pan around a zoomed-in page")),
        SVGReportPrintout::EscapeXmlText(_(L"Double-click")),
        SVGReportPrintout::EscapeXmlText(_(L"Reset zoom")));
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
            canvas->FillChartIds();
            }
        }

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
    // a square page has no distinct second orientation
    const bool dualOrientations{ options.m_dualOrientations &&
                                 pageSize.GetWidth() != pageSize.GetHeight() };
    const wxSize swappedSize{ pageSize.GetHeight(), pageSize.GetWidth() };

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
        { L"gallery", _(L"Gallery") },        { L"story", _(L"Storyline") },
        { L"page", _(L"Page {0}") },          { L"pageOf", _(L"Page {0} of {1}") },
        { L"goTo", _(L"Go to {0}") },         { L"pagesShown", _(L"{0} of {1} pages shown") },
        { L"noPages", _(L"No pages shown") }, { L"chart", _(L"Chart {0}") }
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
                     GetDashboardScriptFilters() + GetDashboardScriptInk() +
                     GetDashboardScriptNavigation() + GetDashboardScriptCounters() +
                     GetDashboardScriptMotion() + GetDashboardScriptZoom() +
                     GetDashboardScriptTooltips() + GetDashboardScriptHelp() +
                     GetDashboardScriptSave() + GetDashboardScriptPrint() +
                     GetDashboardScriptPresent() + GetDashboardScriptThemeMenu() +
                     GetDashboardScriptTheme() + GetDashboardScriptEvents() };
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
                                 SVGReportPrintout::EscapeXmlAttr(wxTheApp->GetAppDisplayName()));
        }
    html += wxString::Format(L"<title>%s</title>\n", SVGReportPrintout::EscapeXmlText(title));
    html += L"<style>\n" + css + L"\n</style>\n";
    html += L"<script>" + script + L"</script>\n";
    html += L"</head>\n<body>\n";

    html += L"<header class=\"dash-toolbar no-print\">\n";
    html += L"<div class=\"dash-brand\">\n";
    // optional logo, embedded as a data URI
    if (const wxString logoUri = BuildLogo(options); !logoUri.empty())
        {
        html += wxString::Format(L"<img class=\"dash-logo\" src=\"%s\" alt=\"\">\n", logoUri);
        }
    html += wxString::Format(L"<h1 class=\"dash-title\">%s</h1>\n</div>\n",
                             SVGReportPrintout::EscapeXmlText(title));
    html += L"<div class=\"dash-controls\">\n";
    html += wxString::Format(
        L"<div class=\"dash-views\" role=\"group\" aria-labelledby=\"dash-views-label\">\n"
        "<span id=\"dash-views-label\" class=\"dash-group-label\">%s</span>\n"
        "<button type=\"button\" data-view=\"gallery\" aria-pressed=\"false\">%s</button>\n"
        "<button type=\"button\" data-view=\"story\" aria-pressed=\"false\">%s</button>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlText(_(L"View")),
        SVGReportPrintout::EscapeXmlText(_(L"Gallery")),
        SVGReportPrintout::EscapeXmlText(_(L"Storyline")));
    if (!distinctLayers.empty())
        {
        html += wxString::Format(
            L"<div class=\"dash-layers\">\n"
            "<button type=\"button\" class=\"dash-layers-btn\" aria-haspopup=\"true\" "
            "aria-expanded=\"false\" aria-controls=\"dash-layers-menu\">%s</button>\n"
            "<div id=\"dash-layers-menu\" class=\"dash-layers-menu\" role=\"group\" "
            "aria-label=\"%s\" hidden>\n",
            SVGReportPrintout::EscapeXmlText(_(L"Layers")),
            SVGReportPrintout::EscapeXmlAttr(_(L"Layers")));
        for (const auto& layer : distinctLayers)
            {
            html += wxString::Format(
                L"<label><input type=\"checkbox\" data-layer=\"%s\" checked>%s</label>\n",
                SVGReportPrintout::EscapeXmlAttr(layer), SVGReportPrintout::EscapeXmlText(layer));
            }
        html += L"</div>\n</div>\n";
        }
    html += wxString::Format(
        L"<div class=\"dash-filters\" hidden>\n"
        "<button type=\"button\" class=\"dash-filters-btn\" aria-haspopup=\"true\" "
        "aria-expanded=\"false\" aria-controls=\"dash-filters-menu\">%s</button>\n"
        "<div id=\"dash-filters-menu\" class=\"dash-filters-menu\" role=\"group\" "
        "aria-label=\"%s\" hidden></div>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlText(_(L"Filters")),
        SVGReportPrintout::EscapeXmlAttr(_(L"Filters")));
    if (options.m_includeColorModeToggle)
        {
        html += wxString::Format(
            L"<div class=\"dash-modes\">\n"
            "<button type=\"button\" id=\"dash-theme\" class=\"dash-theme-btn\" "
            "aria-haspopup=\"menu\" aria-expanded=\"false\" aria-controls=\"dash-theme-menu\" "
            "aria-label=\"%s\"><span class=\"dash-theme-icon\" aria-hidden=\"true\">"
            "◐</span><span class=\"dash-theme-caret\" aria-hidden=\"true\">"
            "▾</span></button>\n"
            "</div>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Theme")));
        }
    html += wxString::Format(
        L"<button type=\"button\" id=\"dash-full\" class=\"dash-full-btn\" hidden "
        "aria-pressed=\"false\" aria-label=\"%s\"><svg class=\"dash-btn-icon\" "
        "viewBox=\"0 0 16 16\" aria-hidden=\"true\"><path d=\"M2.5 6V2.5H6M10 2.5h3.5V6"
        "M13.5 10v3.5H10M6 13.5H2.5V10\"/></svg></button>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Presentation mode")));
    html += wxString::Format(
        L"<button type=\"button\" id=\"dash-print\" class=\"dash-print-btn\" "
        "aria-label=\"%s\"><svg class=\"dash-btn-icon\" viewBox=\"0 0 16 16\" "
        "aria-hidden=\"true\"><path d=\"M4.5 6V2.5h7V6M4.5 11.5h-2v-5h11v5h-2M4.5 9.5h7v4h-7z\"/>"
        "</svg></button>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Print page")));
    html += wxString::Format(
        L"<button type=\"button\" id=\"dash-save\" class=\"dash-save-btn\" "
        "aria-haspopup=\"menu\" aria-expanded=\"false\" aria-controls=\"dash-save-menu\" "
        "aria-label=\"%s\"><svg class=\"dash-btn-icon\" viewBox=\"0 0 16 16\" "
        "aria-hidden=\"true\"><path d=\"M3 2.5h8l2.5 2.5v8.5H3zM5 2.5v3.5h5V2.5M5 13.5v-4h6v4\"/>"
        "</svg></button>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Save page")));
    html += wxString::Format(
        L"<button type=\"button\" id=\"dash-help\" class=\"dash-help-btn\" "
        "aria-haspopup=\"dialog\" aria-expanded=\"false\" aria-controls=\"dash-help-panel\" "
        "aria-label=\"%s\">?</button>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Keyboard and mouse shortcuts")));
    html += L"</div>\n<div class=\"dash-progress\" aria-hidden=\"true\"></div>\n</header>\n";

    html += wxString::Format(
        L"<nav id=\"dash-gallery\" class=\"dash-gallery no-print\" aria-label=\"%s\"></nav>\n"
        "<nav id=\"dash-rail\" class=\"dash-rail no-print\" aria-label=\"%s\"></nav>\n"
        "<div id=\"dash-status\" class=\"visually-hidden\" role=\"status\" "
        "aria-live=\"polite\"></div>\n"
        "<div id=\"dash-tooltip\" class=\"dash-tooltip no-print\" aria-hidden=\"true\"></div>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Pages")),
        SVGReportPrintout::EscapeXmlAttr(_(L"Pages")));

    html += wxString::Format(
        L"<div id=\"dash-save-menu\" class=\"dash-help-panel dash-save-menu no-print\" "
        "role=\"menu\" aria-label=\"%s\" hidden>\n"
        "<button type=\"button\" role=\"menuitem\" data-format=\"svg\">%s</button>\n"
        "<button type=\"button\" role=\"menuitem\" data-format=\"png\">%s</button>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Save page")),
        SVGReportPrintout::EscapeXmlText(_(L"Save as SVG")),
        SVGReportPrintout::EscapeXmlText(_(L"Save as PNG")));

    if (options.m_includeColorModeToggle)
        {
        html += wxString::Format(
            L"<div id=\"dash-theme-menu\" class=\"dash-help-panel dash-save-menu "
            "dash-theme-menu no-print\" role=\"menu\" aria-label=\"%s\" hidden>\n"
            "<button type=\"button\" role=\"menuitemradio\" aria-checked=\"false\" "
            "data-mode=\"auto\"><span class=\"dash-theme-icon\" aria-hidden=\"true\">"
            "◐</span>%s</button>\n"
            "<button type=\"button\" role=\"menuitemradio\" aria-checked=\"false\" "
            "data-mode=\"light\"><span class=\"dash-theme-icon\" aria-hidden=\"true\">"
            "☀</span>%s</button>\n"
            "<button type=\"button\" role=\"menuitemradio\" aria-checked=\"false\" "
            "data-mode=\"dark\"><span class=\"dash-theme-icon\" aria-hidden=\"true\">"
            "☾</span>%s</button>\n"
            "</div>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Theme")),
            SVGReportPrintout::EscapeXmlText(_(L"System settings")),
            SVGReportPrintout::EscapeXmlText(_(L"Light")),
            SVGReportPrintout::EscapeXmlText(_(L"Dark")));
        }

    html += BuildKeyboardTip();

    const wxString loadingText{ _(L"Loading...") };
    html += wxString::Format(
        L"<div class=\"dash-loading no-print\" role=\"progressbar\" aria-label=\"%s\">\n"
        "<div class=\"dash-loading-bar\"></div>\n"
        "<div class=\"dash-loading-label\" aria-hidden=\"true\">%s</div>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlAttr(loadingText),
        SVGReportPrintout::EscapeXmlText(loadingText));

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
        const auto renderPageSvg =
            [canvas, dualOrientations, pageIndex](const wxSize& size, const bool isSwapped)
        {
            const wxString content{ SVGReportPrintout::RenderCanvasToSvg(canvas, size) };
            wxString orientAttrs;
            if (dualOrientations)
                {
                orientAttrs = wxString::Format(
                    L" data-orient=\"%s\" style=\"--svg-w:%d;--svg-h:%d\"",
                    (size.GetWidth() < size.GetHeight()) ? L"portrait" : L"landscape",
                    size.GetWidth(), size.GetHeight());
                }
            return wxString::Format(
                L"<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"page-svg\"%s "
                "viewBox=\"0 0 %d %d\" preserveAspectRatio=\"xMidYMid meet\">\n"
                "<g id=\"page-content-%s%zu\">\n%s\n</g>\n</svg>\n",
                orientAttrs, size.GetWidth(), size.GetHeight(), isSwapped ? L"alt-" : L"",
                pageIndex, content);
        };

        html += wxString::Format(
            L"<section class=\"page\" id=\"page-%zu\" data-index=\"%zu\" data-layer=\"%s\" "
            "aria-label=\"%s\" tabindex=\"-1\">\n",
            pageIndex, pageIndex, SVGReportPrintout::EscapeXmlAttr(canvas->GetLayer()),
            SVGReportPrintout::EscapeXmlAttr(pageTitle));
        html += renderPageSvg(pageSize, false);
        if (dualOrientations)
            {
            html += renderPageSvg(swappedSize, true);
            }
        html += wxString::Format(L"<h2 class=\"page-title\">%s</h2>\n</section>\n",
                                 SVGReportPrintout::EscapeXmlText(pageTitle));
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
