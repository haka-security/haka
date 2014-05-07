$(function()
{
	var navbar = $('#navbar');
	var navTop = $(window).scrollTop() + navbar.offset().top;
	var sibling = $(navbar.siblings()[0]);
	var floating = false;

	function floatingNavbar()
	{
		var scrollTop = $(window).scrollTop();
		var shouldFloat = scrollTop > navTop;

		if (shouldFloat && !floating) {
			navbar.css({
				'position': 'fixed',
				'top': 0,
				'left': navbar.offset().left,
				'width': navbar.width(),
			});
			sibling.css({
				'margin-top': parseInt(sibling.css('margin-top'), 10) + navbar.height(),
			});

			floating = true;
		} else if (!shouldFloat && floating) {
			navbar.css({
				'position': 'static',
			});
			sibling.css({
				'margin-top': parseInt(sibling.css('margin-top'), 10) - navbar.height(),
			});
			floating = false;
		}
	}

	$(window).scroll(floatingNavbar);

	/* Set original state */
	floatingNavbar();
});
