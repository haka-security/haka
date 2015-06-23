/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

$(function () {
	$(".admonition-optional").each(function(index, obj) {
		var title = $(obj).children(".admonition-title");
		$(obj).children(":not(.first)").wrapAll('<div></div>');
		$(title).next().hide();
		$(title).addClass('last');
		$(obj).addClass('closed');

		$(title).click(function () {
			if ($(obj).hasClass('closed')) {
				$(title).toggleClass('last');
				$(obj).toggleClass('closed');
				$(title).next().slideToggle(500);
			}
			else {
				$(title).next().slideToggle(500, function () {
					$(title).toggleClass('last');
					$(obj).toggleClass('closed');
				});
			}
		});
	});
});
