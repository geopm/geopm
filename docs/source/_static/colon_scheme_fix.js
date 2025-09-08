// Copyright (c) 2015 - 2025 Intel Corporation
//  SPDX-License-Identifier: BSD-3-Clause
//
// This script fixes links in the Sphinx search results that contain C++
// symbols with double colons (::).  Browsers interpret the text before the
// first colon as a scheme (like http: or mailto:) and if it is not a known
// scheme, the link is broken.  This script finds such links and prefixes
// them with "./" so that they are treated as relative paths instead of
// schemes.

(function () {
  const SAFE_SCHEMES = new Set(['http:', 'https:', 'mailto:', 'ftp:', 'file:']);
  const IS_PSEUDO_SCHEME = /^[A-Za-z][A-Za-z0-9+.-]*:/;
  const CXX_COLON_HTML = /::.+\.html$/; // quick filter

  function shouldFix(href) {
    if (!href) return false;
    if (!IS_PSEUDO_SCHEME.test(href)) return false;          // no colon early => fine
    const scheme = href.split(':', 1)[0] + ':';
    if (SAFE_SCHEMES.has(scheme)) return false;              // real scheme
    if (!href.endsWith('.html')) return false;               // only our HTML pages
    if (href.indexOf('::') === -1) return false;             // our case uses double colon
    return true;
  }

  function fixAnchor(a) {
    const href = a.getAttribute('href');
    if (shouldFix(href)) {
      // Prefix with ./ so browser treats as relative file path rather than a scheme.
      a.setAttribute('href', './' + href);
    }
  }

  function scan(root) {
    root.querySelectorAll('a[href]').forEach(fixAnchor);
  }

  function init() {
    const results = document.getElementById('search-results');
    if (!results) return; // search page only
    scan(results);
    const obs = new MutationObserver(muts => {
      for (const m of muts) {
        m.addedNodes.forEach(node => {
          if (node.nodeType === 1) {
            if (node.tagName === 'A') fixAnchor(node);
            else scan(node);
          }
        });
      }
    });
    obs.observe(results, { childList: true, subtree: true });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
