
// We draw a triangle with various interpolations

{
	ZBufferPoint *tp, *pr1 = 0, *pr2 = 0, *l1 = 0, *l2 = 0;
	float fdx1, fdx2, fdy1, fdy2, fz, d1, d2;
	unsigned short *pz1;
	unsigned int *pz2;
	PIXEL *pp1;
	int part, update_left, update_right;

	int nb_lines, dx1, dy1, tmp, dx2, dy2;

	int error = 0, derror = 0;
	int x1 = 0, dxdy_min = 0, dxdy_max = 0;
	// warning: x2 is multiplied by 2^16
	int x2 = 0, dx2dy2 = 0;

#ifdef INTERP_Z
	int z1 = 0, dzdx, dzdy, dzdl_min = 0, dzdl_max = 0;
#endif
#ifdef INTERP_RGB
	int r1 = 0, drdx, drdy, drdl_min = 0, drdl_max = 0;
	int g1 = 0, dgdx, dgdy, dgdl_min = 0, dgdl_max = 0;
	int b1 = 0, dbdx, dbdy, dbdl_min = 0, dbdl_max = 0;
#endif
#ifdef INTERP_ST
	int s1 = 0, dsdx, dsdy, dsdl_min = 0, dsdl_max = 0;
	int t1 = 0, dtdx, dtdy, dtdl_min = 0, dtdl_max = 0;
#endif
#ifdef INTERP_STZ
	float sz1, dszdx, dszdy, dszdl_min, dszdl_max;
	float tz1, dtzdx, dtzdy, dtzdl_min, dtzdl_max;
#endif

	// we sort the vertex with increasing y
	if (p1->y < p0->y) {
		tp = p0;
		p0 = p1;
		p1 = tp;
	}
	if (p2->y < p0->y) {
		tp = p2;
		p2 = p1;
		p1 = p0;
		p0 = tp;
	} else if (p2->y < p1->y) {
		tp = p1;
		p1 = p2;
		p2 = tp;
	}

	// we compute dXdx and dXdy for all interpolated values

	fdx1 = (float)(p1->x - p0->x);
	fdy1 = (float)(p1->y - p0->y);

	fdx2 = (float)(p2->x - p0->x);
	fdy2 = (float)(p2->y - p0->y);

	fz = fdx1 * fdy2 - fdx2 * fdy1;
	if (fz == 0)
		return;
	fz = (float)(1.0 / fz);

	fdx1 *= fz;
	fdy1 *= fz;
	fdx2 *= fz;
	fdy2 *= fz;

#ifdef INTERP_Z
	d1 = (float)(p1->z - p0->z);
	d2 = (float)(p2->z - p0->z);
	dzdx = (int)(fdy2 * d1 - fdy1 * d2);
	dzdy = (int)(fdx1 * d2 - fdx2 * d1);
#endif

#ifdef INTERP_RGB
	d1 = (float)(p1->r - p0->r);
	d2 = (float)(p2->r - p0->r);
	drdx = (int)(fdy2 * d1 - fdy1 * d2);
	drdy = (int)(fdx1 * d2 - fdx2 * d1);

	d1 = (float)(p1->g - p0->g);
	d2 = (float)(p2->g - p0->g);
	dgdx = (int)(fdy2 * d1 - fdy1 * d2);
	dgdy = (int)(fdx1 * d2 - fdx2 * d1);

	d1 = (float)(p1->b - p0->b);
	d2 = (float)(p2->b - p0->b);
	dbdx = (int)(fdy2 * d1 - fdy1 * d2);
	dbdy = (int)(fdx1 * d2 - fdx2 * d1);

#endif

#ifdef INTERP_ST
	d1 = (float)(p1->s - p0->s);
	d2 = (float)(p2->s - p0->s);
	dsdx = (int)(fdy2 * d1 - fdy1 * d2);
	dsdy = (int)(fdx1 * d2 - fdx2 * d1);

	d1 = (float)(p1->t - p0->t);
	d2 = (float)(p2->t - p0->t);
	dtdx = (int)(fdy2 * d1 - fdy1 * d2);
	dtdy = (int)(fdx1 * d2 - fdx2 * d1);
#endif

#ifdef INTERP_STZ
	{
		float zz;
		zz = (float)p0->z;
		p0->sz = (float)p0->s * zz;
		p0->tz = (float)p0->t * zz;
		zz = (float)p1->z;
		p1->sz = (float)p1->s * zz;
		p1->tz = (float)p1->t * zz;
		zz=(float) p2->z;
		p2->sz = (float)p2->s * zz;
		p2->tz = (float)p2->t * zz;

		d1 = p1->sz - p0->sz;
		d2 = p2->sz - p0->sz;
		dszdx = (fdy2 * d1 - fdy1 * d2);
		dszdy = (fdx1 * d2 - fdx2 * d1);

		d1 = p1->tz - p0->tz;
		d2 = p2->tz - p0->tz;
		dtzdx = (fdy2 * d1 - fdy1 * d2);
		dtzdy = (fdx1 * d2 - fdx2 * d1);
	}
#endif

	// screen coordinates

	pp1 = (PIXEL *)((char *)zb->pbuf.getRawBuffer() + zb->linesize * p0->y);
	pz1 = zb->zbuf + p0->y * zb->xsize;
	pz2 = zb->zbuf2 + p0->y * zb->xsize;

	DRAW_INIT();

	for (part = 0; part < 2; part++) {
		if (part == 0) {
			if (fz > 0) {
				update_left = 1;
				update_right = 1;
				l1 = p0;
				l2 = p2;
				pr1 = p0;
				pr2 = p1;
			} else {
				update_left = 1;
				update_right = 1;
				l1 = p0;
				l2 = p1;
				pr1 = p0;
				pr2 = p2;
			}
			nb_lines = p1->y - p0->y;
		} else {
			// second part
			if (fz > 0) {
				update_left = 0;
				update_right = 1;
				pr1 = p1;
				pr2 = p2;
			} else {
				update_left = 1;
				update_right = 0;
				l1 = p1;
				l2 = p2;
			}
			nb_lines = p2->y - p1->y + 1;
		}

		// compute the values for the left edge

		if (update_left) {
			dy1 = l2->y - l1->y;
			dx1 = l2->x - l1->x;
			if (dy1 > 0)
				tmp = (dx1 << 16) / dy1;
			else
				tmp = 0;
			x1 = l1->x;
			error = 0;
			derror = tmp & 0x0000ffff;
			dxdy_min = tmp >> 16;
			dxdy_max = dxdy_min + 1;

#ifdef INTERP_Z
			z1 = l1->z;
			dzdl_min = (dzdy + dzdx * dxdy_min);
			dzdl_max = dzdl_min + dzdx;
#endif
#ifdef INTERP_RGB
			r1 = l1->r;
			drdl_min = (drdy + drdx * dxdy_min);
			drdl_max = drdl_min + drdx;

			g1 = l1->g;
			dgdl_min = (dgdy + dgdx * dxdy_min);
			dgdl_max = dgdl_min + dgdx;

			b1 = l1->b;
			dbdl_min = (dbdy + dbdx * dxdy_min);
			dbdl_max = dbdl_min + dbdx;
#endif
#ifdef INTERP_ST
			s1 = l1->s;
			dsdl_min = (dsdy + dsdx * dxdy_min);
			dsdl_max = dsdl_min + dsdx;

			t1 = l1->t;
			dtdl_min = (dtdy + dtdx * dxdy_min);
			dtdl_max = dtdl_min + dtdx;
#endif
#ifdef INTERP_STZ
			sz1 = l1->sz;
			dszdl_min = (dszdy + dszdx * dxdy_min);
			dszdl_max = dszdl_min + dszdx;

			tz1 = l1->tz;
			dtzdl_min = (dtzdy + dtzdx * dxdy_min);
			dtzdl_max = dtzdl_min + dtzdx;
#endif
		}

		// compute values for the right edge

		if (update_right) {
			dx2 = (pr2->x - pr1->x);
			dy2 = (pr2->y - pr1->y);
			if (dy2>0)
				dx2dy2 = ( dx2 << 16) / dy2;
			else
				dx2dy2 = 0;
			x2 = pr1->x << 16;
		}

		// we draw all the scan line of the part

		while (nb_lines>0) {
			nb_lines--;
#ifndef DRAW_LINE
			// generic draw line
			{
				register int n;
#ifdef INTERP_Z
				register unsigned short *pz;
				register unsigned int *pz_2;
				register unsigned int z, zz;
#endif
#ifdef INTERP_RGB
				register unsigned int or1, og1, ob1;
#endif
#ifdef INTERP_ST
				register unsigned int s, t;
#endif
#ifdef INTERP_STZ
				float sz, tz;
#endif

				n = (x2 >> 16) - x1;

				Graphics::PixelBuffer buf = zb->pbuf;
				buf = pp1 + x1 * PSZB;
#ifdef INTERP_Z
				pz = pz1 + x1;
				pz_2 = pz2 + x1;
				z = z1;
#endif
#ifdef INTERP_RGB
				or1 = r1;
				og1 = g1;
				ob1 = b1;
#endif
#ifdef INTERP_ST
				s = s1;
				t = t1;
#endif
#ifdef INTERP_STZ
				sz = sz1;
				tz = tz1;
#endif
				while (n >= 3) {
					PUT_PIXEL(0);
					PUT_PIXEL(1);
					PUT_PIXEL(2);
					PUT_PIXEL(3);
#ifdef INTERP_Z
					pz += 4;
					pz_2 += 4;
#endif
					buf.shiftBy(4);
					n -= 4;
				}
				while (n >= 0) {
					PUT_PIXEL(0);
#ifdef INTERP_Z
					pz += 1;
					pz_2 += 1;
#endif
					buf.shiftBy(1);
					n -= 1;
				}
			}
#else
			DRAW_LINE();
#endif

			// left edge
			error += derror;
			if (error > 0) {
				error -= 0x10000;
				x1 += dxdy_max;
#ifdef INTERP_Z
				z1 += dzdl_max;
#endif
#ifdef INTERP_RGB
				r1 += drdl_max;
				g1 += dgdl_max;
				b1 += dbdl_max;
#endif
#ifdef INTERP_ST
				s1 += dsdl_max;
				t1 += dtdl_max;
#endif
#ifdef INTERP_STZ
				sz1 += dszdl_max;
				tz1 += dtzdl_max;
#endif
			} else {
				x1 += dxdy_min;
#ifdef INTERP_Z
				z1 += dzdl_min;
#endif
#ifdef INTERP_RGB
				r1 += drdl_min;
				g1 += dgdl_min;
				b1 += dbdl_min;
#endif
#ifdef INTERP_ST
				s1 += dsdl_min;
				t1 += dtdl_min;
#endif
#ifdef INTERP_STZ
				sz1 += dszdl_min;
				tz1 += dtzdl_min;
#endif
			}

			// right edge
			x2 += dx2dy2;

			// screen coordinates
			pp1 = (PIXEL *)((char *)pp1 + zb->linesize);
			pz1 += zb->xsize;
			pz2 += zb->xsize;
		}
	}
}

#undef INTERP_Z
#undef INTERP_RGB
#undef INTERP_ST
#undef INTERP_STZ

#undef DRAW_INIT
#undef DRAW_LINE
#undef PUT_PIXEL
