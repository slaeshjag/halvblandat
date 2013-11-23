#include <darnit/darnit.h>

#define	BUF_W		256
#define	BUF_H		256

int main(int argc, char **argv) {
	float t;
	int p0x, p0y, p1x, p1y, p2x, p2y;
	int x, y;
	DARNIT_TILESHEET *ts;
	unsigned int *buf;

	d_init("arne", "arne", NULL);
	ts = d_render_tilesheet_new(1, 1, BUF_W, BUF_H, DARNIT_PFORMAT_RGB5A1);
	buf = calloc(BUF_W * BUF_H, 4);

	p0x = 240;
	p0y = 12;
	p1x = 10;
	p1y = 10;
	p2x = 0;
	p2y = 220;

	p1x -= p0x;
	p1y -= p0y;
	p2x -= p0x;
	p2y -= p0y;

	for (t = 0.0; t < 1.0; t += 0.0005) {
		x = 2.0f * (1.0f - t) * t * p1x + t * t * p2x;
		y = 2.0f * (1.0f - t) * t * p1y + t * t * p2y;
		buf[(y + p0y) * BUF_W + x + p0x] = ~0;
	}

	d_render_tilesheet_update(ts, 0, 0, BUF_W, BUF_H, buf);

	for (;;) {
		d_render_begin();
		d_render_tile_blit(ts, 0, 0, 0);
		d_render_end();
		d_loop();
	}

	return 0;
}
