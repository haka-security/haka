/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var Documentation;

Documentation.highlightSearchWords = function() {
	var params = $.getQueryParameters();
	var terms = (params.highlight) ? params.highlight[0].split(/\s+/) : [];
	if (terms.length) {
		var body = $('#content');
		window.setTimeout(function() {
			$.each(terms, function() {
				body.highlightText(this.toLowerCase(), 'highlighted');
			});
		}, 10);
		$('<a class="highlight-link" href="javascript:Documentation.' +
				'hideSearchWords()" title="' + _('Hide Search Matches') + '"/>')
			.appendTo($('#clean-search-highlight'));
	}
}
