///////////////////////////////////////////////////////////////////////////////
// Name:        htmldashboardprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmldashboardprintout.h"
#include "../graphs/filterablegraph.h"
#include "svgreportprintout.h"
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <wx/app.h>
#include <wx/base64.h>
#include <wx/datetime.h>
#include <wx/dcgraph.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/settings.h>

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
  const hasGroupToggles = {{GROUPTOGGLES}};
  const rememberChoices = {{REMEMBERCHOICES}};
  const layers = {{LAYERS}};
  const activeLayers = new Set(layers);
  let view = '{{VIEW}}';
  let current = 0;
  let pages = [];
  const baseTitle = document.title;

  if (hasModeToggle) {
    try {
      const saved = localStorage.getItem(colorKey);
      if (saved === 'auto' || saved === 'light' || saved === 'dark') colorMode = saved;
    } catch (e) {}
  }

  const stateKey = 'wisteria-dashboard:{{DASHID}}';
  function loadState() {
    try {
      const saved = JSON.parse(localStorage.getItem(stateKey));
      return saved && typeof saved === 'object' ? saved : {};
    } catch (e) { return {}; }
  }
  function saveState() {
    const hidden = layers.filter(function(layer) { return !activeLayers.has(layer); });
    // the choice shown in each filter group of each page with filterable graphs
    const choices = {};
    pages.forEach(function(page, i) {
      if (rememberChoices && page.filters) {
        choices[i] = page.filters.map(function(filter) { return filter.labels[filter.index]; });
      }
    });
    try {
      localStorage.setItem(stateKey,
        JSON.stringify({ view: view, hiddenLayers: hidden, choices: choices }));
    } catch (e) {}
  }
  const savedState = loadState();
  if (Array.isArray(savedState.hiddenLayers)) {
    savedState.hiddenLayers.forEach(function(layer) { activeLayers.delete(layer); });
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptChoices()
    {
    return LR"JS(
  // per-SVG state that is swapped in and out along with a page's filter choice
  const choiceKeys = ['svg', 'alt', 'prepared', 'scopes', 'spotBound', 'zoomBound', 'counters',
                      'countRun', 'inked'];
  function svgState(page, container) {
    const svgs = container.querySelectorAll('.page-svg');
    const state = { svg: svgs[0] || null };
    // a second SVG is the same page in the other orientation
    if (svgs.length > 1) {
      state.alt = { el: page.el, svg: svgs[1], layer: page.layer, title: page.title };
    }
    return state;
  }
  function collectPages() {
    pages = Array.from(document.querySelectorAll('.page')).map(function(el, i) {
      const page = {
        el: el,
        layer: el.getAttribute('data-layer') || '',
        title: el.getAttribute('aria-label') || format(strings.page, i + 1)
      };
      // a page with filterable graphs has a version of itself for each
      // combination of its filter groups' choices
      const choiceEls = el.querySelectorAll(':scope > .page-variant');
      if (!choiceEls.length) {
        Object.assign(page, svgState(page, el));
        return page;
      }
      page.filters = Array.from(el.querySelectorAll(':scope > .page-filter')).map(function(f) {
        return {
          column: f.getAttribute('data-column') || '',
          link: f.getAttribute('data-link') || '',
          labels: Array.from(f.children).map(function(span) { return span.textContent; }),
          index: 0
        };
      });
      page.choices = Array.from(choiceEls).map(function(choiceEl) {
        return {
          el: choiceEl,
          key: choiceEl.getAttribute('data-variant-choices') || '',
          state: svgState(page, choiceEl)
        };
      });
      page.choiceIndex = Math.max(0, page.choices.findIndex(function(choice) {
        return choice.el.hasAttribute('data-variant-default');
      }));
      page.choices[page.choiceIndex].key.split('-').forEach(function(value, i) {
        if (page.filters[i]) page.filters[i].index = parseInt(value, 10) || 0;
      });
      Object.assign(page, page.choices[page.choiceIndex].state);
      return page;
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptChoiceSwitch()
    {
    return LR"JS(
  // shows the version of a page matching its filter groups' choices
  function showPageChoice(page) {
    if (!page.filters) return;
    page.filters.forEach(function(filter) {
      if (!filter.group) return;
      filter.group.querySelectorAll('input[type="radio"]').forEach(function(radio, i) {
        radio.checked = i === filter.index;
      });
    });
    const wanted = page.filters.map(function(filter) { return filter.index; }).join('-');
    const index = page.choices.findIndex(function(choice) { return choice.key === wanted; });
    if (index < 0 || index === page.choiceIndex) return;
    const outgoing = page.choices[page.choiceIndex];
    choiceKeys.forEach(function(key) { outgoing.state[key] = page[key]; });
    outgoing.el.hidden = true;
    const incoming = page.choices[index];
    choiceKeys.forEach(function(key) { page[key] = incoming.state[key]; });
    incoming.el.hidden = false;
    page.choiceIndex = index;
    if (page.card && page.svg) {
      const use = page.card.querySelector('use');
      const group = page.svg.querySelector('g[id^="page-content-"]');
      if (use && group) use.setAttribute('href', '#' + group.id);
    }
    if (inkAssigned && !page.inked) inkPage(page);
    if (page.revealed) {
      // replay the entrance for the newly shown graphs
      page.el.classList.remove('is-entering');
      void page.el.offsetWidth;
      revealPage(page);
    }
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptChoiceMenu()
    {
    return LR"JS(
  function setPageChoice(page, filterIndex, labelIndex) {
    const filter = page.filters && page.filters[filterIndex];
    if (!filter || labelIndex === filter.index || labelIndex >= filter.labels.length) return;
    const label = filter.labels[labelIndex].toLowerCase();
    filter.index = labelIndex;
    showPageChoice(page);
    // filter groups on other pages for the same data and column show the same choice
    if (filter.link) {
      pages.forEach(function(other) {
        if (other === page || !other.filters) return;
        other.filters.forEach(function(otherFilter) {
          if (otherFilter.link !== filter.link) return;
          // the unfiltered choice is always first
          const otherIndex = labelIndex === 0 ? 0 : otherFilter.labels.findIndex(function(text) {
            return text.toLowerCase() === label;
          });
          if (otherIndex >= 0) otherFilter.index = otherIndex;
        });
        showPageChoice(other);
      });
    }
    applyFilters();
    saveState();
  }
  function restoreChoices() {
    const saved = savedState.choices;
    if (!rememberChoices || !saved || typeof saved !== 'object') return;
    pages.forEach(function(page, i) {
      if (!page.filters || !Array.isArray(saved[i])) return;
      saved[i].forEach(function(label, filterIndex) {
        const filter = page.filters[filterIndex];
        const index = filter ? filter.labels.indexOf(label) : -1;
        if (index >= 0) setPageChoice(page, filterIndex, index);
      });
    });
  }
  // each filter group's values, of which one is shown at a time
  function buildChoiceGroups(menu, page, pageIndex) {
    page.filters.forEach(function(filter, filterIndex) {
      const title = filter.column || strings.filter;
      const group = document.createElement('div');
      group.className = 'dash-filters-group';
      group.dataset.page = String(pageIndex);
      group.setAttribute('role', 'radiogroup');
      group.setAttribute('aria-label', title);
      const heading = document.createElement('div');
      heading.className = 'dash-filters-heading';
      heading.textContent = title;
      group.appendChild(heading);
      filter.labels.forEach(function(label, labelIndex) {
        const row = document.createElement('label');
        const radio = document.createElement('input');
        radio.type = 'radio';
        radio.name = 'dash-choice-' + pageIndex + '-' + filterIndex;
        radio.checked = labelIndex === filter.index;
        radio.addEventListener('change', function() {
          if (radio.checked) setPageChoice(page, filterIndex, labelIndex);
        });
        row.appendChild(radio);
        row.appendChild(document.createTextNode(label));
        group.appendChild(row);
      });
      menu.appendChild(group);
      filter.group = group;
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptPages()
    {
    return LR"JS(
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
  function ensureCurrentVisible() {
    const visible = visibleIndexes();
    if (visible.length && visible.indexOf(current) < 0) {
      const next = visible.find(function(i) { return i > current; });
      current = next === undefined ? visible[visible.length - 1] : next;
    }
  }
  function toggleLayer(layer) {
    if (activeLayers.has(layer)) activeLayers.delete(layer);
    else activeLayers.add(layer);
    applyLayers();
    saveState();
    const visible = visibleIndexes();
    ensureCurrentVisible();
    renderView();
    revealCurrent('auto');
    announce(visible.length ? format(strings.pagesShown, visible.length, pages.length) :
                              strings.noPages);
  }
  function readHash(initial) {
    const params = new URLSearchParams(location.hash.slice(1));
    if (initial && pages.length > 1 &&
        (savedState.view === 'gallery' || savedState.view === 'slides')) {
      view = savedState.view;
    }
    const wanted = params.get('view');
    if (pages.length > 1 && (wanted === 'gallery' || wanted === 'slides')) view = wanted;
    const index = parseInt(params.get('page'), 10);
    if (!isNaN(index) && pages.length) {
      current = clampIndex(index);
      // a link to a page always shows that page, even if its layer was hidden
      const target = pages[current];
      if (target.layer && !activeLayers.has(target.layer)) {
        activeLayers.add(target.layer);
        saveState();
      }
    }
  }
  function writeHash() {
    try { history.replaceState(null, '', '#view=' + view + '&page=' + current); } catch (e) {}
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptGallery()
    {
    return LR"JS(
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
    if (view !== 'slides') return;
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
    document.title = pages.length > 1 && pages[current] ?
      pages[current].title + ' - ' + baseTitle : baseTitle;
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptFilters()
    {
    return LR"JS(
  const hiddenFilters = new Set();
  // a filter only affects the chart on its own page
  function filterScopeFor(el) {
    const page = el.closest('.page');
    return (page ? page.dataset.index : '') + ':' + scopeIdFor(el);
  }
  function applyFilters() {
    document.querySelectorAll('.page-svg [data-filter]').forEach(function(el) {
      const key = filterScopeFor(el) + '|' + el.getAttribute('data-filter');
      el.classList.toggle('is-filtered-out', hiddenFilters.has(key));
    });
    // legend rows go away with their data, leaving a gap
    document.querySelectorAll('.page-svg [data-legend-filter]').forEach(function(el) {
      const key = filterScopeFor(el) + '|' + el.getAttribute('data-legend-filter');
      el.classList.toggle('is-filtered-out', hiddenFilters.has(key));
    });
    // nested items (e.g., an inner pie ring) also go away with their parent
    document.querySelectorAll('.page-svg [data-filter-parent], .page-svg [data-legend-parent]')
      .forEach(function(el) {
        const parent = el.getAttribute('data-filter-parent') ||
                       el.getAttribute('data-legend-parent');
        if (hiddenFilters.has(filterScopeFor(el) + '|' + parent)) {
          el.classList.add('is-filtered-out');
        }
      });
    // a base layer goes away once every group in its region is filtered out
    document.querySelectorAll('.page-svg [data-filter-all]').forEach(function(el) {
      const scope = filterScopeFor(el);
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
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptFilterSections()
    {
    return LR"JS(
  function collectFilterSections(page) {
    // a key is either a plain label or "ring", U+2029, and "label" (e.g., for a two-ring pie)
    const fieldSeparator = String.fromCharCode(0x2029);
    const sections = new Map();
    // every graph on the page, in the order it appears, whether or not it has a filter
    const chartIds = [];
    // include every filter choice's graphs (their labels may differ)
    const pageSvgs = page.choices ?
      page.choices.map(function(choice) { return choice.state.svg || page.svg; }) : [page.svg];
    const filterEls = [];
    pageSvgs.forEach(function(svg) {
      svg.querySelectorAll('[data-chart-id]').forEach(function(group) {
        const id = group.getAttribute('data-chart-id');
        if (chartIds.indexOf(id) < 0) chartIds.push(id);
      });
      svg.querySelectorAll('[data-filter]').forEach(function(el) {
        if (!el.closest('[data-filter-graph]')) filterEls.push(el);
      });
    });
    filterEls.forEach(function(el) {
      const chartId = scopeIdFor(el);
      const key = el.getAttribute('data-filter');
      const splitAt = key.indexOf(fieldSeparator);
      const ring = splitAt < 0 ? '' : key.substring(0, splitAt);
      const text = splitAt < 0 ? key : key.substring(splitAt + 1);
      const sectionKey = chartId + fieldSeparator + ring;
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
    return { sections: sections, chartIds: chartIds };
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptFilterMenu()
    {
    return LR"JS(
  function appendFilterGroup(menu, section, chartIds, pageIndex) {
    let title = section.title;
    if (!title) title = format(strings.chart, chartIds.indexOf(section.chartId) + 1);
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
      box.dataset.filterKey = pageIndex + ':' + section.chartId + '|' + entry.key;
      box.addEventListener('change', function() { toggleFilter(box.dataset.filterKey); });
      row.appendChild(box);
      row.appendChild(document.createTextNode(entry.text));
      group.appendChild(row);
    });
    menu.appendChild(group);
  }
  function buildFilters() {
    const menu = document.getElementById('dash-filters-menu');
    if (!menu) return;
    pages.forEach(function(page, pageIndex) {
      if (!page.svg) return;
      if (page.filters) buildChoiceGroups(menu, page, pageIndex);
      if (!hasGroupToggles) return;
      const found = collectFilterSections(page);
      found.sections.forEach(function(section) {
        appendFilterGroup(menu, section, found.chartIds, pageIndex);
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
  let inkAssigned = false;
  function tagPageInk(page) {
    try { tagInk(page); } catch (e) {}
    try { if (page.alt) tagInk(page.alt); } catch (e) {}
    page.inked = true;
  }
  function assignInk() {
    const hidden = pages.filter(function(page) { return page.el.getClientRects().length === 0; });
    hidden.forEach(function(page) { page.el.classList.add('ink-measure'); });
    pages.forEach(tagPageInk);
    hidden.forEach(function(page) { page.el.classList.remove('ink-measure'); });
    inkAssigned = true;
  }
  // a page's filter choice that was hidden when ink was first assigned
  function inkPage(page) {
    const hidden = page.el.getClientRects().length === 0;
    if (hidden) page.el.classList.add('ink-measure');
    tagPageInk(page);
    if (hidden) page.el.classList.remove('ink-measure');
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
      saveState();
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
      if (view !== 'slides') return;
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptMarks()
    {
    return LR"JS(
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
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptMotion()
    {
    return LR"JS(
  const shapeSelector = 'circle, ellipse, rect, path, polygon, polyline, line';
  const maxAnimatedMarks = 2500;

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
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptSpotlight()
    {
    return LR"JS(
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
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptReveal()
    {
    return LR"JS(
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
        const scope = filterScopeFor(hit);
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptSaveCopy()
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
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptSave()
    {
    return LR"JS(
  function savePageAsSvg(page) {
    const clone = clonePageSvg(page, pageContentSize(visibleSvg(page)));
    if (!clone) return;
    const xml = '<?xml version="1.0" encoding="UTF-8"?>\n' +
      new XMLSerializer().serializeToString(clone);
    downloadBlob(new Blob([xml], { type: 'image/svg+xml' }), slugify(page.title) + '.svg');
  }
  // calls done with the page as a PNG blob, or null if it couldn't be drawn
  function renderPagePng(page, done) {
    const size = pageContentSize(visibleSvg(page));
    const clone = clonePageSvg(page, size);
    if (!clone) {
      done(null);
      return;
    }
    const xml = new XMLSerializer().serializeToString(clone);
    const scale = window.devicePixelRatio || 1;
    const img = new Image();
    img.onload = function() {
      const canvas = document.createElement('canvas');
      canvas.width = Math.round(size.width * scale);
      canvas.height = Math.round(size.height * scale);
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
      canvas.toBlob(done, 'image/png');
    };
    img.onerror = function() { done(null); };
    img.src = 'data:image/svg+xml;charset=utf-8;base64,' +
      btoa(unescape(encodeURIComponent(xml)));
  }
  function savePageAsPng(page) {
    renderPagePng(page, function(blob) {
      if (blob) downloadBlob(blob, slugify(page.title) + '.png');
    });
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptSaveMenu()
    {
    return LR"JS(
  // a toolbar button's drop-down menu of actions, returning the function that closes it
  function bindActionMenu(btnId, menuId, setOpen) {
    const btn = document.getElementById(btnId);
    const menu = document.getElementById(menuId);
    if (!btn || !menu) return null;
    let isOpen = false;
    function positionMenu() {
      const rect = btn.getBoundingClientRect();
      const menuRect = menu.getBoundingClientRect();
      const left = Math.min(Math.max(8, rect.right - menuRect.width),
        window.innerWidth - menuRect.width - 8);
      menu.style.left = left + 'px';
      menu.style.top = (rect.bottom + 8) + 'px';
    }
    function onOutsidePointerDown(e) {
      if (menu.contains(e.target) || btn.contains(e.target)) return;
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
      isOpen = true;
      setOpen(true);
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
      if (!isOpen) return;
      isOpen = false;
      setOpen(false);
      menu.classList.remove('is-open');
      btn.setAttribute('aria-expanded', 'false');
      document.removeEventListener('keydown', onMenuKeyDown);
      document.removeEventListener('pointerdown', onOutsidePointerDown, true);
      window.removeEventListener('resize', positionMenu);
      window.setTimeout(function() { if (!isOpen) menu.hidden = true; }, 150);
      if (returnFocus) btn.focus();
    }
    btn.addEventListener('click', function() {
      if (isOpen) closeMenu(true); else openMenu();
    });
    return closeMenu;
  }
  function bindSave() {
    const closeMenu = bindActionMenu('dash-save', 'dash-save-menu',
                                     function(open) { saveOpen = open; });
    if (!closeMenu) return;
    document.querySelectorAll('#dash-save-menu button[data-format]').forEach(function(item) {
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
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptKeys()
    {
    return LR"JS(
  function onKeyDown(e) {
    // F1 opens and closes the shortcuts panel instead of the browser's help
    if (e.key === 'F1' && !e.defaultPrevented && !e.ctrlKey && !e.metaKey && !e.altKey) {
      const helpBtn = document.getElementById('dash-help');
      if (!helpBtn) return;
      e.preventDefault();
      if (!saveOpen && !copyOpen && !themeOpen) helpBtn.click();
      return;
    }
    if (e.defaultPrevented || e.ctrlKey || e.metaKey || e.altKey || !pages.length ||
        helpOpen || saveOpen || copyOpen || themeOpen) {
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
    } else if ((e.key === 'g' || e.key === 'G') && document.querySelector('.dash-views')) {
      e.preventDefault();
      setView(view === 'gallery' ? 'slides' : 'gallery');
    } else if (e.key >= '1' && e.key <= '9' && e.key.length === 1) {
      const target = visibleIndexes()[Number(e.key) - 1];
      if (target !== undefined) {
        e.preventDefault();
        goTo(target, true);
      }
    }
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptTouch()
    {
    return LR"JS(
  function bindSwipe() {
    const container = document.querySelector('.dash-pages');
    if (!container) return;
    let startX = 0;
    let startY = 0;
    let tracking = false;
    container.addEventListener('touchstart', function(e) {
      const svg = e.target.closest ? e.target.closest('.page-svg') : null;
      // dragging a zoomed-in page pans it instead
      tracking = view === 'slides' && e.touches.length === 1 &&
        !(svg && svg.classList.contains('is-zoomed'));
      if (!tracking) return;
      startX = e.touches[0].clientX;
      startY = e.touches[0].clientY;
    }, { passive: true });
    container.addEventListener('touchmove', function(e) {
      if (e.touches.length > 1) tracking = false;
    }, { passive: true });
    container.addEventListener('touchend', function(e) {
      if (!tracking || !e.changedTouches.length) return;
      tracking = false;
      const dx = e.changedTouches[0].clientX - startX;
      const dy = e.changedTouches[0].clientY - startY;
      if (Math.abs(dx) > 60 && Math.abs(dx) > 1.5 * Math.abs(dy)) stepPage(dx < 0 ? 1 : -1);
    }, { passive: true });
  }
  let copyOpen = false;
  // briefly shows on a menu item that its copy worked, then calls done
  function flashCopied(item, text, done) {
    const original = item.textContent;
    item.textContent = text;
    announce(text);
    window.setTimeout(function() {
      item.textContent = original;
      done();
    }, 1200);
  }
  function bindCopy() {
    const closeMenu = bindActionMenu('dash-copy', 'dash-copy-menu',
                                     function(open) { copyOpen = open; });
    if (!closeMenu) return;
    const finish = function() { closeMenu(true); };
    const imageItem = document.querySelector('#dash-copy-menu [data-copy="png"]');
    if (imageItem && (!navigator.clipboard || !navigator.clipboard.write ||
                      !window.ClipboardItem)) {
      imageItem.remove();
    } else if (imageItem) {
      imageItem.addEventListener('click', function() {
        const page = pages[current];
        if (!page) return;
        // the clipboard item is made during the click and gets its image once it is drawn
        const png = new Promise(function(resolve, reject) {
          renderPagePng(page, function(blob) { if (blob) resolve(blob); else reject(); });
        });
        navigator.clipboard.write([new ClipboardItem({ 'image/png': png })]).then(
          function() { flashCopied(imageItem, strings.imageCopied, finish); }, finish);
      });
    }
    const linkItem = document.querySelector('#dash-copy-menu [data-copy="link"]');
    if (linkItem) {
      linkItem.addEventListener('click', function() {
        if (!navigator.clipboard) {
          finish();
          return;
        }
        navigator.clipboard.writeText(location.href).then(
          function() { flashCopied(linkItem, strings.linkCopied, finish); }, finish);
      });
    }
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptMenus()
    {
    return LR"JS(
  function bindMenu(btnSelector, menuId, wrapperSelector) {
    const btn = document.querySelector(btnSelector);
    const menu = document.getElementById(menuId);
    if (!btn || !menu) return;
    // a menu too wide for the room right of its button shifts left to stay in the window
    const fitMenu = function() {
      menu.style.left = '';
      if (menu.hidden || getComputedStyle(menu).position !== 'absolute') return;
      const rect = menu.getBoundingClientRect();
      const overflow = rect.right - (document.documentElement.clientWidth - 8);
      if (overflow > 0) {
        menu.style.left = -Math.min(overflow, Math.max(0, rect.left - 8)) + 'px';
      }
    };
    const setMenu = function(open) {
      menu.hidden = !open;
      btn.setAttribute('aria-expanded', open ? 'true' : 'false');
      fitMenu();
    };
    window.addEventListener('resize', fitMenu);
    btn.addEventListener('click', function() { setMenu(menu.hidden); });
    const menuBoxes = function() {
      return Array.prototype.filter.call(
        menu.querySelectorAll('input[type="checkbox"], input[type="radio"]'),
        function(box) { return box.offsetParent !== null; });
    };
    // up and down arrows move through the checkboxes and radio buttons,
    // and Enter toggles the focused one
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
      if (ev.key === 'Enter' &&
          ev.target.matches('input[type="checkbox"], input[type="radio"]')) {
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
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptEvents()
    {
    return LR"JS(
  function bindControls() {
    document.querySelectorAll('.dash-views button').forEach(function(btn) {
      btn.addEventListener('click', function() { setView(btn.dataset.view); });
    });
    document.querySelectorAll('.dash-layers input[data-layer]').forEach(function(box) {
      box.addEventListener('change', function() { toggleLayer(box.dataset.layer); });
    });
    bindMenu('.dash-layers-btn', 'dash-layers-menu', '.dash-layers');
    bindMenu('.dash-filters-btn', 'dash-filters-menu', '.dash-filters');
    document.addEventListener('keydown', onKeyDown);
    window.addEventListener('scroll', updateProgress, { passive: true });
    window.addEventListener('resize', measureChrome);
    bindTooltips();
    bindHelp();
    bindSave();
    bindCopy();
    bindPrint();
    bindPresent();
    bindTheme();
    bindSwipe();
  }
)JS";
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetDashboardScriptLoad()
    {
    return LR"JS(
  document.addEventListener('DOMContentLoaded', function() {
    // Remove the native-tooltip sources only.
    // Keep chart <desc> elements, screen readers need them.
    document.querySelectorAll('.page-svg title').forEach(function(n) { n.remove(); });
    document.querySelectorAll('.page-svg > g[id^="page-content"] > desc').forEach(function(n) { n.remove(); });
    collectPages();
    readHash(true);
    applyColorMode();
    buildRail();
    buildGallery();
    bindControls();
    applyLayers();
    ensureCurrentVisible();
    buildFilters();
    restoreChoices();
    applyFilters();
    renderView();
    revealCurrent('auto');
    window.addEventListener('hashchange', function() {
      readHash(false);
      applyLayers();
      withTransition(function() {
        renderView();
        revealCurrent('auto');
      });
    });
    observeStory();
    // let the loading indicator paint before the heavy work starts
    window.requestAnimationFrame(function() {
      window.setTimeout(function() {
        assignInk();
        setupMotion();
        if (!visibleIndexes().length || !root.classList.contains('motion-ready')) markReady();
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
wxString Wisteria::HtmlDashboardPrintout::BuildKeyboardTip(const bool singlePage)
    {
    // a lone page has no other pages or gallery view to go to
    const wxString pageRows{
        singlePage ?
            wxString{} :
            wxString::Format(
                L"<dt><kbd>&larr;</kbd> <kbd>&rarr;</kbd></dt><dd>%s</dd>\n"
                "<dt><kbd>Home</kbd> <kbd>End</kbd></dt><dd>%s</dd>\n"
                "<dt><kbd>1</kbd> &ndash; <kbd>9</kbd></dt><dd>%s</dd>\n"
                "<dt><kbd>G</kbd></dt><dd>%s</dd>\n",
                SVGReportPrintout::EscapeXmlText(_(L"Go to the previous or next page")),
                SVGReportPrintout::EscapeXmlText(_(L"Jump to the first or last page")),
                SVGReportPrintout::EscapeXmlText(_(L"Jump to a page")),
                SVGReportPrintout::EscapeXmlText(_(L"Switch between gallery and slideshow")))
    };
    const wxString swipeRow{
        singlePage ? wxString{} :
                     wxString::Format(L"<dt class=\"dash-touch-only\">%s</dt>"
                                      "<dd class=\"dash-touch-only\">%s</dd>\n",
                                      SVGReportPrintout::EscapeXmlText(_(L"Swipe")),
                                      SVGReportPrintout::EscapeXmlText(
                                          _(L"Go to the previous or next page (touch screens)")))
    };
    return wxString::Format(
        L"<div id=\"dash-help-panel\" class=\"dash-help-panel no-print\" role=\"dialog\" "
        "aria-modal=\"false\" aria-labelledby=\"dash-help-title\" hidden>\n"
        "<div class=\"dash-help-header\">\n"
        "<h2 id=\"dash-help-title\">%s</h2>\n"
        "<button type=\"button\" id=\"dash-help-close\" aria-label=\"%s\">&times;</button>\n"
        "</div>\n"
        "<dl class=\"dash-help-list\">\n"
        "%s"
        "<dt><kbd>F</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>&uarr;</kbd> <kbd>&darr;</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>Space</kbd> <kbd>Enter</kbd></dt><dd>%s</dd>\n"
        "<dt><kbd>Esc</kbd></dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "<dt>%s</dt><dd>%s</dd>\n"
        "%s"
        "</dl>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlText(_(L"Keyboard & mouse shortcuts")),
        SVGReportPrintout::EscapeXmlAttr(_(L"Close")), pageRows,
        SVGReportPrintout::EscapeXmlText(_(L"Enter or leave fullscreen presentation mode")),
        SVGReportPrintout::EscapeXmlText(_(L"Move through the items of a menu (Tab also works)")),
        SVGReportPrintout::EscapeXmlText(_(L"Check or uncheck the selected item in a menu")),
        SVGReportPrintout::EscapeXmlText(_(L"Close the open menu")),
        SVGReportPrintout::EscapeXmlText(_(L"Ctrl") + L"+" + _(L"scroll") + L" / " + _(L"pinch")),
        SVGReportPrintout::EscapeXmlText(_(L"Zoom in or out, centered on the cursor")),
        SVGReportPrintout::EscapeXmlText(_(L"Drag")),
        SVGReportPrintout::EscapeXmlText(_(L"Pan around a zoomed-in page")),
        SVGReportPrintout::EscapeXmlText(_(L"Double-click")),
        SVGReportPrintout::EscapeXmlText(_(L"Reset zoom")), swipeRow);
    }

//------------------------------------------------------
std::vector<Wisteria::HtmlDashboardPrintout::FilterableGraphs>
Wisteria::HtmlDashboardPrintout::GetPageFilters(const Canvas& canvas)
    {
    std::vector<FilterableGraphs> filterGroups;
    for (const auto& filterableGraph : canvas.GetFilterableGraphs())
        {
        if (filterableGraph->GetVariants().size() < 2)
            {
            continue;
            }
        const auto foundGroup = std::ranges::find_if(
            filterGroups,
            [&filterableGraph](const auto& filterGroup)
            {
                return filterGroup.front()->GetData() == filterableGraph->GetData() &&
                       filterGroup.front()->GetFilterColumn().CmpNoCase(
                           filterableGraph->GetFilterColumn()) == 0;
            });
        if (foundGroup == filterGroups.end())
            {
            filterGroups.push_back(FilterableGraphs{ filterableGraph });
            }
        else
            {
            foundGroup->push_back(filterableGraph);
            }
        }
    return filterGroups;
    }

//------------------------------------------------------
std::vector<std::vector<std::optional<size_t>>>
Wisteria::HtmlDashboardPrintout::GetFilterLinks(const std::vector<Canvas*>& canvases)
    {
    // each filter group is keyed by the dataset and column it filters
    std::vector<std::pair<const Data::Dataset*, wxString>> linkKeys;
    std::vector<size_t> pageCounts;
    std::vector<std::vector<std::optional<size_t>>> pageLinks(canvases.size());
    for (size_t i = 0; i < canvases.size(); ++i)
        {
        if (canvases[i] == nullptr)
            {
            continue;
            }
        for (const auto& filterGroup : GetPageFilters(*canvases[i]))
            {
            const Data::Dataset* groupData{ filterGroup.front()->GetData().get() };
            const wxString& column{ filterGroup.front()->GetFilterColumn() };
            const auto foundKey = std::ranges::find_if(
                linkKeys, [groupData, &column](const auto& linkKey)
                { return linkKey.first == groupData && linkKey.second.CmpNoCase(column) == 0; });
            if (foundKey == linkKeys.cend())
                {
                pageLinks[i].emplace_back(linkKeys.size());
                linkKeys.emplace_back(groupData, column);
                pageCounts.push_back(1);
                }
            else
                {
                const auto keyIndex{ static_cast<size_t>(
                    std::distance(linkKeys.begin(), foundKey)) };
                pageLinks[i].emplace_back(keyIndex);
                ++pageCounts[keyIndex];
                }
            }
        }
    // a filter group that isn't shared with another page isn't linked
    for (auto& groupLinks : pageLinks)
        {
        for (auto& groupLink : groupLinks)
            {
            if (groupLink.has_value() && pageCounts[groupLink.value()] < 2)
                {
                groupLink.reset();
                }
            }
        }
    return pageLinks;
    }

//------------------------------------------------------
std::vector<Wisteria::HtmlDashboardOptions::FilterDefault>
Wisteria::HtmlDashboardPrintout::GetFilterDefaults(const std::vector<Canvas*>& canvases)
    {
    std::vector<HtmlDashboardOptions::FilterDefault> filterDefaults;
    for (const auto* canvas : canvases)
        {
        if (canvas == nullptr)
            {
            continue;
            }
        for (const auto& pageFilters : GetPageFilters(*canvas))
            {
            const Data::Dataset* groupData{ pageFilters.front()->GetData().get() };
            const wxString& column{ pageFilters.front()->GetFilterColumn() };
            std::vector<wxString> choices{ GetFilterChoices(pageFilters) };

            const auto foundDefault =
                std::ranges::find_if(filterDefaults,
                                     [groupData, &column](const auto& filterDefault)
                                     {
                                         return filterDefault.m_data == groupData &&
                                                filterDefault.m_column.CmpNoCase(column) == 0;
                                     });
            if (foundDefault == filterDefaults.end())
                {
                HtmlDashboardOptions::FilterDefault filterDefault;
                filterDefault.m_data = groupData;
                filterDefault.m_column = column;
                filterDefault.m_datasetName =
                    (groupData != nullptr) ? wxString{ groupData->GetName() } : wxString{};
                filterDefault.m_defaultChoice = GetDefaultFilterChoice(pageFilters, choices);
                filterDefault.m_choices = std::move(choices);
                filterDefaults.push_back(std::move(filterDefault));
                continue;
                }

            // another page's graphs may offer values that this page's don't
            for (size_t i = 1; i < choices.size(); ++i)
                {
                const wxString& label{ choices[i] };
                const bool isKnown{ std::ranges::any_of(
                    foundDefault->m_choices,
                    [&label](const auto& existing) { return existing.CmpNoCase(label) == 0; }) };
                if (!isKnown)
                    {
                    foundDefault->m_choices.push_back(label);
                    }
                }
            // the unfiltered choice stays first and the rest are kept sorted, with the
            // default choice tracked by its label
            const wxString defaultLabel{ foundDefault->m_choices[foundDefault->m_defaultChoice] };
            std::sort(std::next(foundDefault->m_choices.begin()), foundDefault->m_choices.end(),
                      [](const auto& lhv, const auto& rhv) { return lhv.CmpNoCase(rhv) < 0; });
            const auto defaultPos =
                std::ranges::find_if(foundDefault->m_choices, [&defaultLabel](const auto& label)
                                     { return label.CmpNoCase(defaultLabel) == 0; });
            foundDefault->m_defaultChoice =
                static_cast<size_t>(std::distance(foundDefault->m_choices.begin(), defaultPos));
            }
        }
    return filterDefaults;
    }

//------------------------------------------------------
std::vector<Wisteria::HtmlDashboardPrintout::FilterGroup>
Wisteria::HtmlDashboardPrintout::GetFilterGroups(
    const Canvas& canvas, const std::vector<std::optional<size_t>>& groupLinks)
    {
    std::vector<FilterGroup> filterGroups;
    for (auto& pageFilters : GetPageFilters(canvas))
        {
        FilterGroup filterGroup;
        for (const auto& filterableGraph : pageFilters)
            {
            const auto legend = filterableGraph->GetLegend();
            filterGroup.m_originalStates.push_back(
                FilterState{ filterableGraph->GetActiveIndex(), filterableGraph->IsShown(),
                             (legend == nullptr || legend->IsShown()) });
            }
        filterGroup.m_choices = GetFilterChoices(pageFilters);
        // the group opens on the choice being shown on the canvas
        filterGroup.m_defaultChoice = GetDefaultFilterChoice(pageFilters, filterGroup.m_choices);
        if (filterGroups.size() < groupLinks.size())
            {
            filterGroup.m_link = groupLinks[filterGroups.size()];
            }
        filterGroup.m_graphs = std::move(pageFilters);
        filterGroups.push_back(std::move(filterGroup));
        }
    return filterGroups;
    }

//------------------------------------------------------
void Wisteria::HtmlDashboardPrintout::ApplyLinkedChoice(FilterGroup& filterGroup,
                                                        const LinkedChoice& firstChoice,
                                                        const wxString& pageTitle)
    {
    const auto& choices{ filterGroup.m_choices };
    // the unfiltered choice is always first, but its label may differ
    const auto linkedPos =
        firstChoice.m_isAll ?
            choices.cbegin() :
            std::ranges::find_if(choices, [&firstChoice](const auto& label)
                                 { return label.CmpNoCase(firstChoice.m_label) == 0; });
    if (linkedPos == choices.cend())
        {
        return;
        }
    const auto linkedIndex{ static_cast<size_t>(std::distance(choices.cbegin(), linkedPos)) };
    if (linkedIndex != filterGroup.m_defaultChoice)
        {
        wxLogWarning(L"The '%s' filter on page '%s' opens on '%s' instead of '%s' "
                     "to match the linked filter on page '%s'.",
                     filterGroup.m_graphs.front()->GetFilterColumn(), pageTitle, *linkedPos,
                     choices[filterGroup.m_defaultChoice], firstChoice.m_pageTitle);
        filterGroup.m_defaultChoice = linkedIndex;
        }
    }

//------------------------------------------------------
wxString
Wisteria::HtmlDashboardPrintout::GetFilterGroupTitle(const std::vector<FilterGroup>& filterGroups,
                                                     const size_t groupIndex)
    {
    const auto& firstGraph = filterGroups[groupIndex].m_graphs.front();
    const wxString& column{ firstGraph->GetFilterColumn() };
    const bool hasSharedColumn{
        std::ranges::count_if(
            filterGroups, [&column](const auto& filterGroup)
            { return filterGroup.m_graphs.front()->GetFilterColumn().CmpNoCase(column) == 0; }) > 1
    };
    if (!hasSharedColumn)
        {
        return column;
        }
    const wxString datasetName{ (firstGraph->GetData() != nullptr) ?
                                    wxString{ firstGraph->GetData()->GetName() } :
                                    wxString{} };
    return wxString::Format(L"%s (%s)", column,
                            datasetName.empty() ? wxString::Format(L"%zu", groupIndex + 1) :
                                                  datasetName);
    }

//------------------------------------------------------
std::vector<wxString>
Wisteria::HtmlDashboardPrintout::GetFilterChoices(const FilterableGraphs& pageFilters)
    {
    std::vector<wxString> choices;
    for (const auto& filterableGraph : pageFilters)
        {
        // the first variant is the unfiltered one, which is added at the front afterward
        const auto& filteredGraphs = filterableGraph->GetVariants();
        for (size_t i = 1; i < filteredGraphs.size(); ++i)
            {
            const wxString& label{ filteredGraphs[i].m_label };
            if (std::ranges::none_of(choices, [&label](const auto& existing)
                                     { return existing.CmpNoCase(label) == 0; }))
                {
                choices.push_back(label);
                }
            }
        }
    std::ranges::sort(choices,
                      [](const auto& lhv, const auto& rhv) { return lhv.CmpNoCase(rhv) < 0; });
    choices.insert(choices.begin(), pageFilters.front()->GetVariants().front().m_label);
    return choices;
    }

//------------------------------------------------------
size_t Wisteria::HtmlDashboardPrintout::GetDefaultFilterChoice(const FilterableGraphs& pageFilters,
                                                               const std::vector<wxString>& choices)
    {
    const auto& firstFilter = pageFilters.front();
    if (firstFilter->GetActiveIndex() == 0)
        {
        return 0;
        }
    const wxString& activeLabel{
        firstFilter->GetVariants().at(firstFilter->GetActiveIndex()).m_label
    };
    const auto defaultPos = std::ranges::find_if(choices, [&activeLabel](const auto& label)
                                                 { return label.CmpNoCase(activeLabel) == 0; });
    return (defaultPos == choices.cend()) ?
               0 :
               static_cast<size_t>(std::distance(choices.cbegin(), defaultPos));
    }

//------------------------------------------------------
void Wisteria::HtmlDashboardPrintout::ShowFilterChoice(
    const FilterableGraphs& pageFilters, const std::vector<FilterState>& originalStates,
    const wxString& choice, const bool isAllChoice)
    {
    for (size_t i = 0; i < pageFilters.size(); ++i)
        {
        const auto& filterableGraph = pageFilters[i];
        const std::optional<size_t> choiceIndex{ isAllChoice ?
                                                     std::optional<size_t>{ 0 } :
                                                     filterableGraph->FindVariant(choice) };
        // undo any blanking from the previous choice before switching
        filterableGraph->Show(originalStates[i].m_shown);
        if (const auto legend = filterableGraph->GetLegend(); legend != nullptr)
            {
            legend->Show(originalStates[i].m_legendShown);
            }
        filterableGraph->SetActiveIndex(choiceIndex.value_or(originalStates[i].m_activeIndex));

        // a graph without this choice is left blank, rather than showing another value's data
        if (!choiceIndex.has_value())
            {
            filterableGraph->Show(false);
            if (const auto legend = filterableGraph->GetLegend(); legend != nullptr)
                {
                legend->Show(false);
                }
            }
        }
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::ScopeSvgIds(const wxString& svg, const wxString& scope)
    {
    constexpr std::wstring_view ID_ATTR{ L" id=\"" };
    constexpr std::wstring_view URL_REF{ L"url(#" };
    constexpr std::wstring_view HREF_REF{ L"href=\"#" };

    const std::wstring source{ svg.ToStdWstring() };
    const std::wstring_view text{ source };
    const std::wstring scopePrefix{ scope.ToStdWstring() + L"-" };

    std::wstring scoped;
    scoped.reserve(text.size() + (text.size() / 16));
    std::map<std::wstring, std::wstring, std::less<>> latestIds;
    std::map<std::wstring, size_t, std::less<>> idCounts;

    // the next position of each marker, only searched for again once it has been passed
    size_t idPos{ text.find(ID_ATTR) };
    size_t urlPos{ text.find(URL_REF) };
    size_t hrefPos{ text.find(HREF_REF) };

    size_t pos{ 0 };
    while (pos < text.size())
        {
        if (idPos != std::wstring_view::npos && idPos < pos)
            {
            idPos = text.find(ID_ATTR, pos);
            }
        if (urlPos != std::wstring_view::npos && urlPos < pos)
            {
            urlPos = text.find(URL_REF, pos);
            }
        if (hrefPos != std::wstring_view::npos && hrefPos < pos)
            {
            hrefPos = text.find(HREF_REF, pos);
            }
        const size_t markerPos{ std::min({ idPos, urlPos, hrefPos }) };
        if (markerPos == std::wstring_view::npos)
            {
            break;
            }

        const bool isDefinition{ markerPos == idPos };
        const size_t nameStart{ markerPos + (isDefinition        ? ID_ATTR.size() :
                                             markerPos == urlPos ? URL_REF.size() :
                                                                   HREF_REF.size()) };
        const size_t nameEnd{ text.find((markerPos == urlPos) ? L')' : L'"', nameStart) };
        if (nameEnd == std::wstring_view::npos)
            {
            break;
            }
        const std::wstring_view name{ text.substr(nameStart, nameEnd - nameStart) };

        scoped.append(text.substr(pos, nameStart - pos));
        if (isDefinition)
            {
            const size_t timesSeen{ idCounts[std::wstring{ name }]++ };
            std::wstring newId{ scopePrefix };
            newId.append(name);
            if (timesSeen > 0)
                {
                newId.append(L"-").append(std::to_wstring(timesSeen));
                }
            scoped.append(newId);
            latestIds.insert_or_assign(std::wstring{ name }, std::move(newId));
            }
        else if (const auto foundId = latestIds.find(name); foundId != latestIds.cend())
            {
            scoped.append(foundId->second);
            }
        else
            {
            scoped.append(scopePrefix).append(name);
            }
        pos = nameEnd;
        }
    if (pos < text.size())
        {
        scoped.append(text.substr(pos));
        }

    return wxString{ scoped };
    }

//------------------------------------------------------
void Wisteria::HtmlDashboardPrintout::RestoreFilterStates(
    const FilterableGraphs& pageFilters, const std::vector<FilterState>& originalStates)
    {
    for (size_t i = 0; i < pageFilters.size(); ++i)
        {
        pageFilters[i]->SetActiveIndex(originalStates[i].m_activeIndex);
        pageFilters[i]->Show(originalStates[i].m_shown);
        if (const auto legend = pageFilters[i]->GetLegend(); legend != nullptr)
            {
            legend->Show(originalStates[i].m_legendShown);
            }
        }
    }

//------------------------------------------------------
wxString Wisteria::HtmlDashboardPrintout::GetFilterCaption(const Canvas& canvas)
    {
    std::vector<wxString> descriptions;
    for (const auto& filterableGraph : canvas.GetFilterableGraphs())
        {
        const auto& filteredGraphs = filterableGraph->GetVariants();
        if (!filterableGraph->IsShown() ||
            filterableGraph->GetActiveIndex() >= filteredGraphs.size())
            {
            continue;
            }
        const wxString columnName{
            L"<tspan font-weight=\"bold\">" +
            SVGReportPrintout::EscapeXmlText(filterableGraph->GetFilterColumn()) + L"</tspan>"
        };
        const wxString description{
            (filterableGraph->GetActiveIndex() == 0) ?
                wxString::Format(SVGReportPrintout::EscapeXmlText(_(L"%s unfiltered")),
                                 columnName) :
                wxString::Format(
                    SVGReportPrintout::EscapeXmlText(_(L"%s filtered by %s")), columnName,
                    L"<tspan font-style=\"italic\">" +
                        SVGReportPrintout::EscapeXmlText(
                            filteredGraphs[filterableGraph->GetActiveIndex()].m_label) +
                        L"</tspan>")
        };
        // graphs switched together by the same value are only described once
        if (std::ranges::none_of(descriptions, [&description](const auto& existing)
                                 { return existing.CmpNoCase(description) == 0; }))
            {
            descriptions.push_back(description);
            }
        }
    if (descriptions.empty())
        {
        return {};
        }
    wxString joined;
    for (const auto& description : descriptions)
        {
        if (!joined.empty())
            {
            joined += L", ";
            }
        joined += description;
        }
    return wxString::Format(SVGReportPrintout::EscapeXmlText(_(L"Filters: %s")), joined);
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

    // every page sets aside room below its graphs for a filter caption,
    // so that all pages stay the same size
    const bool hasFilterCaptions{ std::ranges::any_of(
        canvases, [](const auto* canvas)
        { return canvas != nullptr && !canvas->GetFilterableGraphs().empty(); }) };
    constexpr static int CAPTION_FONT_SIZE{ 14 };
    const int captionHeight{ hasFilterCaptions ? CAPTION_FONT_SIZE * 2 : 0 };
    const wxString captionFont{ SVGReportPrintout::EscapeXmlAttr(
        wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName()) };

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

    // a lone page is always shown as a slide, without the view toggle or page rail
    const bool singlePage{ std::count_if(canvases.cbegin(), canvases.cend(), [](const auto* canvas)
                                         { return canvas != nullptr; }) == 1 };
    const wxString initialView{ HtmlDashboardOptions::ViewToString(
        singlePage ? HtmlDashboardOptions::DashboardView::Slides : options.m_view) };

    // user-facing text used by the script
    const std::vector<std::pair<wxString, wxString>> scriptStrings{
        { L"gallery", _(L"Gallery") },         { L"slides", _(L"Slideshow") },
        { L"page", _(L"Page {0}") },           { L"pageOf", _(L"Page {0} of {1}") },
        { L"goTo", _(L"Go to {0}") },          { L"pagesShown", _(L"{0} of {1} pages shown") },
        { L"noPages", _(L"No pages shown") },  { L"chart", _(L"Chart {0}") },
        { L"filter", _(L"Filter") },           { L"linkCopied", _(L"Link copied") },
        { L"imageCopied", _(L"Image copied") }
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

    // identifies this dashboard so that saved viewer settings are not shared with others
    uint32_t dashboardHash{ 2166136261U };
    const auto hashText = [&dashboardHash](const wxString& text)
    {
        for (const auto ch : text)
            {
            dashboardHash ^= static_cast<uint32_t>(ch);
            dashboardHash *= 16777619U;
            }
        dashboardHash ^= 0xFFU;
        dashboardHash *= 16777619U;
    };
    hashText(options.m_title);
    hashText(std::to_wstring(canvases.size()));
    for (const auto& layer : distinctLayers)
        {
        hashText(layer);
        }
    // a different default filter choice must not be overridden by a viewer's saved choice
    for (const auto& filterDefault : GetFilterDefaults(canvases))
        {
        const auto foundOverride = std::ranges::find_if(
            options.m_filterOverrides,
            [&filterDefault](const auto& filterOverride)
            {
                return filterOverride.m_data == filterDefault.m_data &&
                       filterOverride.m_column.CmpNoCase(filterDefault.m_column) == 0;
            });
        hashText(filterDefault.m_column);
        hashText((foundOverride != options.m_filterOverrides.cend()) ?
                     foundOverride->m_label :
                     filterDefault.m_choices.at(filterDefault.m_defaultChoice));
        }
    const wxString dashboardId{ wxString::Format(L"%08x", dashboardHash) };

    // the state script opens the enclosing function and the load script closes it
    wxString script{
        GetDashboardScriptState() + GetDashboardScriptChoices() + GetDashboardScriptChoiceSwitch() +
        GetDashboardScriptChoiceMenu() + GetDashboardScriptPages() + GetDashboardScriptGallery() +
        GetDashboardScriptFilters() + GetDashboardScriptFilterSections() +
        GetDashboardScriptFilterMenu() + GetDashboardScriptInk() + GetDashboardScriptNavigation() +
        GetDashboardScriptCounters() + GetDashboardScriptMarks() + GetDashboardScriptMotion() +
        GetDashboardScriptSpotlight() + GetDashboardScriptReveal() + GetDashboardScriptZoom() +
        GetDashboardScriptTooltips() + GetDashboardScriptHelp() + GetDashboardScriptSaveCopy() +
        GetDashboardScriptSave() + GetDashboardScriptSaveMenu() + GetDashboardScriptPrint() +
        GetDashboardScriptPresent() + GetDashboardScriptThemeMenu() + GetDashboardScriptTheme() +
        GetDashboardScriptKeys() + GetDashboardScriptTouch() + GetDashboardScriptMenus() +
        GetDashboardScriptEvents() + GetDashboardScriptLoad()
    };
    script.Replace(L"{{TOGGLE}}", options.m_includeColorModeToggle ? L"true" : L"false");
    script.Replace(L"{{COUNTUP}}", options.m_countUpNumbers ? L"true" : L"false");
    script.Replace(L"{{GROUPTOGGLES}}",
                   options.m_includeGroupVisibilityToggles ? L"true" : L"false");
    script.Replace(L"{{REMEMBERCHOICES}}", options.m_rememberFilterChoices ? L"true" : L"false");
    script.Replace(L"{{MODE}}", initialMode);
    script.Replace(L"{{VIEW}}", initialView);
    script.Replace(L"{{DASHID}}", dashboardId);
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
    html += wxString::Format(L"<meta name=\"dcterms.created\" content=\"%s\">\n",
                             wxDateTime::Now().FormatISOCombined());
    if (!options.m_author.empty())
        {
        html += wxString::Format(L"<meta name=\"author\" content=\"%s\">\n",
                                 SVGReportPrintout::EscapeXmlAttr(options.m_author));
        }
    if (!options.m_subject.empty())
        {
        html += wxString::Format(L"<meta name=\"description\" content=\"%s\">\n",
                                 SVGReportPrintout::EscapeXmlAttr(options.m_subject));
        }
    if (!options.m_keywords.empty())
        {
        html += wxString::Format(L"<meta name=\"keywords\" content=\"%s\">\n",
                                 SVGReportPrintout::EscapeXmlAttr(options.m_keywords));
        }
    html += wxString::Format(L"<title>%s</title>\n", SVGReportPrintout::EscapeXmlText(title));
    // optional logo, embedded as a data URI
    const wxString logoUri{ BuildLogo(options) };
    if (!logoUri.empty())
        {
        html += wxString::Format(L"<link rel=\"icon\" href=\"%s\">\n", logoUri);
        }
    html += L"<style>\n" + css + L"\n</style>\n";
    html += L"<script>" + script + L"</script>\n";
    html += L"</head>\n<body>\n";

    html += L"<header class=\"dash-toolbar no-print\">\n";
    html += L"<div class=\"dash-brand\">\n";
    if (!logoUri.empty())
        {
        html += wxString::Format(L"<img class=\"dash-logo\" src=\"%s\" alt=\"\">\n", logoUri);
        }
    html += wxString::Format(L"<h1 class=\"dash-title\">%s</h1>\n</div>\n",
                             SVGReportPrintout::EscapeXmlText(title));
    html += L"<div class=\"dash-controls\">\n";
    if (!singlePage)
        {
        html += wxString::Format(
            L"<div class=\"dash-views\" role=\"group\" aria-labelledby=\"dash-views-label\">\n"
            "<span id=\"dash-views-label\" class=\"dash-group-label\">%s</span>\n"
            "<button type=\"button\" data-view=\"gallery\" aria-pressed=\"false\">%s</button>\n"
            "<button type=\"button\" data-view=\"slides\" aria-pressed=\"false\">%s</button>\n"
            "</div>\n",
            SVGReportPrintout::EscapeXmlText(_(L"View")),
            SVGReportPrintout::EscapeXmlText(_(L"Gallery")),
            SVGReportPrintout::EscapeXmlText(_(L"Slideshow")));
        }
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
    if (options.m_includePrint)
        {
        html +=
            wxString::Format(L"<button type=\"button\" id=\"dash-print\" class=\"dash-print-btn\" "
                             "aria-label=\"%s\"><svg class=\"dash-btn-icon\" viewBox=\"0 0 16 16\" "
                             "aria-hidden=\"true\"><path d=\"M4.5 6V2.5h7V6M4.5 "
                             "11.5h-2v-5h11v5h-2M4.5 9.5h7v4h-7z\"/>"
                             "</svg></button>\n",
                             SVGReportPrintout::EscapeXmlAttr(_(L"Print page")));
        }
    if (options.m_includeCopy)
        {
        html += wxString::Format(
            L"<button type=\"button\" id=\"dash-copy\" class=\"dash-copy-btn\" "
            "aria-haspopup=\"menu\" aria-expanded=\"false\" aria-controls=\"dash-copy-menu\" "
            "aria-label=\"%s\" title=\"%s\"><svg class=\"dash-btn-icon\" viewBox=\"0 0 16 16\" "
            "aria-hidden=\"true\"><path d=\"M5.5 5.5h8v8h-8zM10.5 5.5v-3h-8v8h3\"/>"
            "</svg></button>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Copy")),
            SVGReportPrintout::EscapeXmlAttr(_(L"Copy")));
        }
    if (options.m_includeSave)
        {
        html += wxString::Format(
            L"<button type=\"button\" id=\"dash-save\" class=\"dash-save-btn\" "
            "aria-haspopup=\"menu\" aria-expanded=\"false\" aria-controls=\"dash-save-menu\" "
            "aria-label=\"%s\"><svg class=\"dash-btn-icon\" viewBox=\"0 0 16 16\" "
            "aria-hidden=\"true\"><path d=\"M3 2.5h8l2.5 2.5v8.5H3zM5 2.5v3.5h5V2.5M5 "
            "13.5v-4h6v4\"/>"
            "</svg></button>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Save page")));
        }
    html += wxString::Format(
        L"<button type=\"button\" id=\"dash-help\" class=\"dash-help-btn\" "
        "aria-haspopup=\"dialog\" aria-expanded=\"false\" aria-controls=\"dash-help-panel\" "
        "aria-label=\"%s\">?</button>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Keyboard and mouse shortcuts")));
    html += L"</div>\n<div class=\"dash-progress\" aria-hidden=\"true\"></div>\n</header>\n";

    html += wxString::Format(
        L"<nav id=\"dash-gallery\" class=\"dash-gallery no-print\" aria-label=\"%s\"></nav>\n",
        SVGReportPrintout::EscapeXmlAttr(_(L"Pages")));
    if (!singlePage)
        {
        html += wxString::Format(
            L"<nav id=\"dash-rail\" class=\"dash-rail no-print\" aria-label=\"%s\"></nav>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Pages")));
        }
    html +=
        L"<div id=\"dash-status\" class=\"visually-hidden\" role=\"status\" "
        "aria-live=\"polite\"></div>\n"
        "<div id=\"dash-tooltip\" class=\"dash-tooltip no-print\" aria-hidden=\"true\"></div>\n";

    if (options.m_includeSave)
        {
        html += wxString::Format(
            L"<div id=\"dash-save-menu\" class=\"dash-help-panel dash-save-menu no-print\" "
            "role=\"menu\" aria-label=\"%s\" hidden>\n"
            "<button type=\"button\" role=\"menuitem\" data-format=\"svg\">%s</button>\n"
            "<button type=\"button\" role=\"menuitem\" data-format=\"png\">%s</button>\n"
            "</div>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Save page")),
            SVGReportPrintout::EscapeXmlText(_(L"Save as SVG")),
            SVGReportPrintout::EscapeXmlText(_(L"Save as PNG")));
        }

    if (options.m_includeCopy)
        {
        html += wxString::Format(
            L"<div id=\"dash-copy-menu\" class=\"dash-help-panel dash-save-menu no-print\" "
            "role=\"menu\" aria-label=\"%s\" hidden>\n"
            "<button type=\"button\" role=\"menuitem\" data-copy=\"png\">%s</button>\n"
            "<button type=\"button\" role=\"menuitem\" data-copy=\"link\">%s</button>\n"
            "</div>\n",
            SVGReportPrintout::EscapeXmlAttr(_(L"Copy")),
            SVGReportPrintout::EscapeXmlText(_(L"Copy as image")),
            SVGReportPrintout::EscapeXmlText(_(L"Copy page link")));
        }

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

    html += BuildKeyboardTip(singlePage);

    const wxString loadingText{ _(L"Loading...") };
    html += wxString::Format(
        L"<div class=\"dash-loading no-print\" role=\"progressbar\" aria-label=\"%s\">\n"
        "<div class=\"dash-loading-bar\"></div>\n"
        "<div class=\"dash-loading-label\" aria-hidden=\"true\">%s</div>\n"
        "</div>\n",
        SVGReportPrintout::EscapeXmlAttr(loadingText),
        SVGReportPrintout::EscapeXmlText(loadingText));

    html += wxString::Format(L"<main class=\"dash-pages\" style=\"--page-w:%d;--page-h:%d\">\n",
                             pageSize.GetWidth(), pageSize.GetHeight() + captionHeight);
    // linked filter groups open on the choice of the first group in their link
    const std::vector<std::vector<std::optional<size_t>>> filterLinks{ GetFilterLinks(canvases) };
    std::map<size_t, LinkedChoice> linkedChoices;
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
        const auto renderPageSvg = [canvas, dualOrientations, pageIndex, captionHeight,
                                    &captionFont](const wxSize& size, const bool isSwapped,
                                                  const wxString& idSuffix = wxString{})
        {
            // every render (page, orientation and filter choice) gets its own element IDs
            const wxString content{ ScopeSvgIds(
                SVGReportPrintout::RenderCanvasToSvg(canvas, size),
                wxString::Format(L"p%zu%s%s", pageIndex, isSwapped ? L"-alt" : L"", idSuffix)) };
            const int fullHeight{ size.GetHeight() + captionHeight };
            wxString orientAttrs;
            if (dualOrientations)
                {
                orientAttrs = wxString::Format(
                    L" data-orient=\"%s\" style=\"--svg-w:%d;--svg-h:%d\"",
                    (size.GetWidth() < size.GetHeight()) ? L"portrait" : L"landscape",
                    size.GetWidth(), fullHeight);
                }
            wxString caption;
            if (const wxString captionText{ GetFilterCaption(*canvas) }; !captionText.empty())
                {
                caption =
                    wxString::Format(L"<text class=\"page-filter-caption\" x=\"%d\" y=\"%d\" "
                                     "dy=\"0.35em\" fill=\"#000000\" font-family=\"%s\" "
                                     "font-size=\"%d\">%s</text>\n",
                                     CAPTION_FONT_SIZE, size.GetHeight() + (captionHeight / 2),
                                     captionFont, CAPTION_FONT_SIZE, captionText);
                }
            return wxString::Format(
                L"<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"page-svg\"%s "
                "viewBox=\"0 0 %d %d\" preserveAspectRatio=\"xMidYMid meet\">\n"
                "<g id=\"page-content-%s%zu%s\">\n%s\n%s</g>\n</svg>\n",
                orientAttrs, size.GetWidth(), fullHeight, isSwapped ? L"alt-" : L"", pageIndex,
                idSuffix, content, caption);
        };

        std::vector<FilterGroup> filterGroups{ GetFilterGroups(*canvas, filterLinks[canvasIndex]) };
        for (auto& filterGroup : filterGroups)
            {
            const Data::Dataset* groupData{ filterGroup.m_graphs.front()->GetData().get() };
            const wxString& column{ filterGroup.m_graphs.front()->GetFilterColumn() };
            const auto foundOverride =
                std::ranges::find_if(options.m_filterOverrides,
                                     [groupData, &column](const auto& filterOverride)
                                     {
                                         return filterOverride.m_data == groupData &&
                                                filterOverride.m_column.CmpNoCase(column) == 0;
                                     });
            if (foundOverride == options.m_filterOverrides.cend())
                {
                continue;
                }
            const auto overridePos =
                std::ranges::find_if(filterGroup.m_choices, [&foundOverride](const auto& label)
                                     { return label.CmpNoCase(foundOverride->m_label) == 0; });
            if (overridePos != filterGroup.m_choices.end())
                {
                filterGroup.m_defaultChoice =
                    static_cast<size_t>(std::distance(filterGroup.m_choices.begin(), overridePos));
                }
            }

        html += wxString::Format(
            L"<section class=\"page\" id=\"page-%zu\" data-index=\"%zu\" data-guid=\"%s\" "
            "data-layer=\"%s\" aria-label=\"%s\" tabindex=\"-1\">\n",
            pageIndex, pageIndex, SVGReportPrintout::EscapeXmlAttr(canvas->GetGuid()),
            SVGReportPrintout::EscapeXmlAttr(canvas->GetLayer()),
            SVGReportPrintout::EscapeXmlAttr(pageTitle));
        if (filterGroups.empty())
            {
            html += renderPageSvg(pageSize, false);
            if (dualOrientations)
                {
                html += renderPageSvg(swappedSize, true);
                }
            }
        else
            {
            // each filter group is described for the filter menu
            for (size_t groupIndex = 0; groupIndex < filterGroups.size(); ++groupIndex)
                {
                auto& filterGroup = filterGroups[groupIndex];
                if (filterGroup.m_link.has_value())
                    {
                    if (const auto linkedChoice = linkedChoices.find(filterGroup.m_link.value());
                        linkedChoice != linkedChoices.cend())
                        {
                        ApplyLinkedChoice(filterGroup, linkedChoice->second, pageTitle);
                        }
                    else
                        {
                        linkedChoices.try_emplace(
                            filterGroup.m_link.value(),
                            LinkedChoice{ filterGroup.m_choices[filterGroup.m_defaultChoice],
                                          filterGroup.m_defaultChoice == 0, pageTitle });
                        }
                    }
                html += wxString::Format(
                    L"<div class=\"page-filter\" data-column=\"%s\" data-link=\"%s\" hidden>",
                    SVGReportPrintout::EscapeXmlAttr(GetFilterGroupTitle(filterGroups, groupIndex)),
                    filterGroup.m_link.has_value() ?
                        wxString::Format(L"%zu", filterGroup.m_link.value()) :
                        wxString{});
                for (const auto& choice : filterGroup.m_choices)
                    {
                    html += L"<span>" + SVGReportPrintout::EscapeXmlText(choice) + L"</span>";
                    }
                html += L"</div>\n";
                }

            // a version of the page for every combination of the groups' choices
            std::vector<size_t> combination(filterGroups.size(), 0);
            while (true)
                {
                bool isDefault{ true };
                wxString choiceIndexes;
                for (size_t i = 0; i < filterGroups.size(); ++i)
                    {
                    const auto& filterGroup = filterGroups[i];
                    ShowFilterChoice(filterGroup.m_graphs, filterGroup.m_originalStates,
                                     filterGroup.m_choices[combination[i]], combination[i] == 0);
                    isDefault = isDefault && (combination[i] == filterGroup.m_defaultChoice);
                    choiceIndexes +=
                        wxString::Format(L"%s%zu", (i == 0) ? L"" : L"-", combination[i]);
                    }
                // the version shown first keeps the page's plain IDs
                const wxString idSuffix{ isDefault ? wxString{} : L"-v" + choiceIndexes };
                html += wxString::Format(
                    L"<div class=\"page-variant\" data-variant-choices=\"%s\"%s>\n", choiceIndexes,
                    isDefault ? L" data-variant-default" : L" hidden");
                html += renderPageSvg(pageSize, false, idSuffix);
                if (dualOrientations)
                    {
                    html += renderPageSvg(swappedSize, true, idSuffix);
                    }
                html += L"</div>\n";

                // step to the next combination, the first group's choice changing fastest
                size_t groupIndex{ 0 };
                while (groupIndex < combination.size() &&
                       ++combination[groupIndex] == filterGroups[groupIndex].m_choices.size())
                    {
                    combination[groupIndex] = 0;
                    ++groupIndex;
                    }
                if (groupIndex == combination.size())
                    {
                    break;
                    }
                }

            for (const auto& filterGroup : filterGroups)
                {
                RestoreFilterStates(filterGroup.m_graphs, filterGroup.m_originalStates);
                }
                // lay out the canvas again for the graphs it was showing before exporting
                {
                wxGCDC gdc{ canvas };
                canvas->CalcAllSizes(gdc);
                }
            canvas->Refresh();
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
        Settings::ReportError(
            wxString::Format(_(L"Failed to save HTML dashboard to \"%s\"."), options.m_filePath),
            _(L"Export Error"));
        }
    }
