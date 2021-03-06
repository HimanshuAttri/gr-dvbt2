/* -*- c++ -*- */
/* 
 * Copyright 2014 Ron Economos.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "interleaver_bb_impl.h"
#include <stdio.h>

namespace gr {
  namespace dvbt2 {

    interleaver_bb::sptr
    interleaver_bb::make(dvbt2_framesize_t framesize, dvbt2_code_rate_t rate, dvbt2_constellation_t constellation)
    {
      return gnuradio::get_initial_sptr
        (new interleaver_bb_impl(framesize, rate, constellation));
    }

    /*
     * The private constructor
     */
    interleaver_bb_impl::interleaver_bb_impl(dvbt2_framesize_t framesize, dvbt2_code_rate_t rate, dvbt2_constellation_t constellation)
      : gr::block("interleaver_bb",
              gr::io_signature::make(1, 1, sizeof(unsigned char)),
              gr::io_signature::make(1, 1, sizeof(unsigned char)))
    {
        signal_constellation = constellation;
        code_rate = rate;
        if (framesize == gr::dvbt2::FECFRAME_NORMAL)
        {
            frame_size = FRAME_SIZE_NORMAL;
            switch (rate)
            {
                case gr::dvbt2::C1_3:
                case gr::dvbt2::C2_5:
                case gr::dvbt2::C1_2:
                    nbch = 32400;
                    q_val = 90;
                    break;
                case gr::dvbt2::C3_5:
                    nbch = 38880;
                    q_val = 72;
                    break;
                case gr::dvbt2::C2_3:
                    nbch = 43200;
                    q_val = 60;
                    break;
                case gr::dvbt2::C3_4:
                    nbch = 48600;
                    q_val = 45;
                    break;
                case gr::dvbt2::C4_5:
                    nbch = 51840;
                    q_val = 36;
                    break;
                case gr::dvbt2::C5_6:
                    nbch = 54000;
                    q_val = 30;
                    break;
            }
        }
        else
        {
            frame_size = FRAME_SIZE_SHORT;
            switch (rate)
            {
                case gr::dvbt2::C1_3:
                    nbch = 5400;
                    q_val = 30;
                    break;
                case gr::dvbt2::C2_5:
                    nbch = 6480;
                    q_val = 27;
                    break;
                case gr::dvbt2::C1_2:
                    nbch = 7200;
                    q_val = 25;
                    break;
                case gr::dvbt2::C3_5:
                    nbch = 9720;
                    q_val = 18;
                    break;
                case gr::dvbt2::C2_3:
                    nbch = 10800;
                    q_val = 15;
                    break;
                case gr::dvbt2::C3_4:
                    nbch = 11880;
                    q_val = 12;
                    break;
                case gr::dvbt2::C4_5:
                    nbch = 12600;
                    q_val = 10;
                    break;
                case gr::dvbt2::C5_6:
                    nbch = 13320;
                    q_val = 8;
                    break;
            }
        }
        switch (constellation)
        {
            case gr::dvbt2::MOD_QPSK:
                mod = 2;
                set_output_multiple(frame_size / mod);
                packed_items = frame_size / mod;
                break;
            case gr::dvbt2::MOD_16QAM:
                mod = 4;
                set_output_multiple(frame_size / mod);
                packed_items = frame_size / mod;
                break;
            case gr::dvbt2::MOD_64QAM:
                mod = 6;
                set_output_multiple(frame_size / mod);
                packed_items = frame_size / mod;
                break;
            case gr::dvbt2::MOD_256QAM:
                mod = 8;
                set_output_multiple(frame_size / mod);
                packed_items = frame_size / mod;
                break;
        }
    }

    /*
     * Our virtual destructor.
     */
    interleaver_bb_impl::~interleaver_bb_impl()
    {
    }

    void
    interleaver_bb_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        ninput_items_required[0] = noutput_items * mod;
    }

    int
    interleaver_bb_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const unsigned char *in = (const unsigned char *) input_items[0];
        unsigned char *out = (unsigned char *) output_items[0];
        int consumed = 0;
        int produced = 0;
        int rows, offset, index;
        unsigned int pack;
        const int *twist;
        const int *mux;

        switch (signal_constellation)
        {
            case gr::dvbt2::MOD_QPSK:
                for (int i = 0; i < noutput_items; i += packed_items)
                {
                    rows = frame_size / 2;
                    if (code_rate == gr::dvbt2::C1_3 || code_rate == gr::dvbt2::C2_5)
                    {
                        for (int k = 0; k < nbch; k++)
                        {
                            tempu[k] = *in++;
                        }
                        for (int t = 0; t < q_val; t++)
                        {
                            for (int s = 0; s < 360; s++)
                            {
                                tempu[nbch + (360 * t) + s] = in[(q_val * s) + t];
                            }
                        }
                        in = in + (q_val * 360);
                        index = 0;
                        for (int j = 0; j < rows; j++)
                        {
                            out[produced] = tempu[index++] << 1;
                            out[produced++] |= tempu[index++];
                            consumed += 2;
                        }
                    }
                    else
                    {
                        for (int j = 0; j < rows; j++)
                        {
                            out[produced] = in[consumed++] << 1;
                            out[produced++] |= in[consumed++];
                        }
                    }
                }
                break;
            case gr::dvbt2::MOD_16QAM:
                if (frame_size == FRAME_SIZE_NORMAL)
                {
                    twist = &twist16n[0];
                }
                else
                {
                    twist = &twist16s[0];
                }
                if (code_rate == gr::dvbt2::C3_5 && frame_size == FRAME_SIZE_NORMAL)
                {
                    mux = &mux16_35[0];
                }
                else if (code_rate == gr::dvbt2::C1_3 && frame_size == FRAME_SIZE_SHORT)
                {
                    mux = &mux16_13[0];
                }
                else if (code_rate == gr::dvbt2::C2_5 && frame_size == FRAME_SIZE_SHORT)
                {
                    mux = &mux16_25[0];
                }
                else
                {
                    mux = &mux16[0];
                }
                for (int i = 0; i < noutput_items; i += packed_items)
                {
                    rows = frame_size / (mod * 2);
                    const unsigned char *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8;
                    c1 = &tempv[0];
                    c2 = &tempv[rows];
                    c3 = &tempv[rows * 2];
                    c4 = &tempv[rows * 3];
                    c5 = &tempv[rows * 4];
                    c6 = &tempv[rows * 5];
                    c7 = &tempv[rows * 6];
                    c8 = &tempv[rows * 7];
                    for (int k = 0; k < nbch; k++)
                    {
                        tempu[k] = *in++;
                    }
                    for (int t = 0; t < q_val; t++)
                    {
                        for (int s = 0; s < 360; s++)
                        {
                            tempu[nbch + (360 * t) + s] = in[(q_val * s) + t];
                        }
                    }
                    in = in + (q_val * 360);
                    index = 0;
                    for (int col = 0; col < (mod * 2); col++)
                    {
                        offset = twist[col];
                        for (int row = 0; row < rows; row++)
                        {
                            tempv[offset + (rows * col)] = tempu[index++];
                            offset = (offset + 1) % rows;
                        }
                    }
                    index = 0;
                    for (int j = 0; j < rows; j++)
                    {
                        tempu[index++] = c1[j];
                        tempu[index++] = c2[j];
                        tempu[index++] = c3[j];
                        tempu[index++] = c4[j];
                        tempu[index++] = c5[j];
                        tempu[index++] = c6[j];
                        tempu[index++] = c7[j];
                        tempu[index++] = c8[j];
                    }
                    index = 0;
                    for (int d = 0; d < frame_size / (mod * 2); d++)
                    {
                        pack = 0;
                        for (int e = 0; e < (mod * 2); e++)
                        {
                            offset = mux[e];
                            pack |= tempu[index + offset];
                            pack <<= 1;
                        }
                        pack >>= 1;
                        out[produced++] = pack >> 4;
                        out[produced++] = pack & 0xf;
                        index += (mod * 2);
                        consumed += (mod * 2);
                    }
                }
                break;
            case gr::dvbt2::MOD_64QAM:
                if (frame_size == FRAME_SIZE_NORMAL)
                {
                    twist = &twist64n[0];
                }
                else
                {
                    twist = &twist64s[0];
                }
                if (code_rate == gr::dvbt2::C3_5 && frame_size == FRAME_SIZE_NORMAL)
                {
                    mux = &mux64_35[0];
                }
                else if (code_rate == gr::dvbt2::C1_3 && frame_size == FRAME_SIZE_SHORT)
                {
                    mux = &mux64_13[0];
                }
                else if (code_rate == gr::dvbt2::C2_5 && frame_size == FRAME_SIZE_SHORT)
                {
                    mux = &mux64_25[0];
                }
                else
                {
                    mux = &mux64[0];
                }
                for (int i = 0; i < noutput_items; i += packed_items)
                {
                    rows = frame_size / (mod * 2);
                    const unsigned char *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9, *c10, *c11, *c12;
                    c1 = &tempv[0];
                    c2 = &tempv[rows];
                    c3 = &tempv[rows * 2];
                    c4 = &tempv[rows * 3];
                    c5 = &tempv[rows * 4];
                    c6 = &tempv[rows * 5];
                    c7 = &tempv[rows * 6];
                    c8 = &tempv[rows * 7];
                    c9 = &tempv[rows * 8];
                    c10 = &tempv[rows * 9];
                    c11 = &tempv[rows * 10];
                    c12 = &tempv[rows * 11];
                    for (int k = 0; k < nbch; k++)
                    {
                        tempu[k] = *in++;
                    }
                    for (int t = 0; t < q_val; t++)
                    {
                        for (int s = 0; s < 360; s++)
                        {
                            tempu[nbch + (360 * t) + s] = in[(q_val * s) + t];
                        }
                    }
                    in = in + (q_val * 360);
                    index = 0;
                    for (int col = 0; col < (mod * 2); col++)
                    {
                        offset = twist[col];
                        for (int row = 0; row < rows; row++)
                        {
                            tempv[offset + (rows * col)] = tempu[index++];
                            offset = (offset + 1) % rows;
                        }
                    }
                    index = 0;
                    for (int j = 0; j < rows; j++)
                    {
                        tempu[index++] = c1[j];
                        tempu[index++] = c2[j];
                        tempu[index++] = c3[j];
                        tempu[index++] = c4[j];
                        tempu[index++] = c5[j];
                        tempu[index++] = c6[j];
                        tempu[index++] = c7[j];
                        tempu[index++] = c8[j];
                        tempu[index++] = c9[j];
                        tempu[index++] = c10[j];
                        tempu[index++] = c11[j];
                        tempu[index++] = c12[j];
                    }
                    index = 0;
                    for (int d = 0; d < frame_size / (mod * 2); d++)
                    {
                        pack = 0;
                        for (int e = 0; e < (mod * 2); e++)
                        {
                            offset = mux[e];
                            pack |= tempu[index + offset];
                            pack <<= 1;
                        }
                        pack >>= 1;
                        out[produced++] = pack >> 6;
                        out[produced++] = pack & 0x3f;
                        index += (mod * 2);
                        consumed += (mod * 2);
                    }
                }
                break;
            case gr::dvbt2::MOD_256QAM:
                if (frame_size == FRAME_SIZE_NORMAL)
                {
                    if (code_rate == gr::dvbt2::C3_5)
                    {
                        mux = &mux256_35[0];
                    }
                    else if (code_rate == gr::dvbt2::C2_3)
                    {
                        mux = &mux256_23[0];
                    }
                    else
                    {
                        mux = &mux256[0];
                    }
                    for (int i = 0; i < noutput_items; i += packed_items)
                    {
                        rows = frame_size / (mod * 2);
                        const unsigned char *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8;
                        const unsigned char *c9, *c10, *c11, *c12, *c13, *c14, *c15, *c16;
                        c1 = &tempv[0];
                        c2 = &tempv[rows];
                        c3 = &tempv[rows * 2];
                        c4 = &tempv[rows * 3];
                        c5 = &tempv[rows * 4];
                        c6 = &tempv[rows * 5];
                        c7 = &tempv[rows * 6];
                        c8 = &tempv[rows * 7];
                        c9 = &tempv[rows * 8];
                        c10 = &tempv[rows * 9];
                        c11 = &tempv[rows * 10];
                        c12 = &tempv[rows * 11];
                        c13 = &tempv[rows * 12];
                        c14 = &tempv[rows * 13];
                        c15 = &tempv[rows * 14];
                        c16 = &tempv[rows * 15];
                        for (int k = 0; k < nbch; k++)
                        {
                            tempu[k] = *in++;
                        }
                        for (int t = 0; t < q_val; t++)
                        {
                            for (int s = 0; s < 360; s++)
                            {
                                tempu[nbch + (360 * t) + s] = in[(q_val * s) + t];
                            }
                        }
                        in = in + (q_val * 360);
                        index = 0;
                        for (int col = 0; col < (mod * 2); col++)
                        {
                            offset = twist256n[col];
                            for (int row = 0; row < rows; row++)
                            {
                                tempv[offset + (rows * col)] = tempu[index++];
                                offset = (offset + 1) % rows;
                            }
                        }
                        index = 0;
                        for (int j = 0; j < rows; j++)
                        {
                            tempu[index++] = c1[j];
                            tempu[index++] = c2[j];
                            tempu[index++] = c3[j];
                            tempu[index++] = c4[j];
                            tempu[index++] = c5[j];
                            tempu[index++] = c6[j];
                            tempu[index++] = c7[j];
                            tempu[index++] = c8[j];
                            tempu[index++] = c9[j];
                            tempu[index++] = c10[j];
                            tempu[index++] = c11[j];
                            tempu[index++] = c12[j];
                            tempu[index++] = c13[j];
                            tempu[index++] = c14[j];
                            tempu[index++] = c15[j];
                            tempu[index++] = c16[j];
                        }
                        index = 0;
                        for (int d = 0; d < frame_size / (mod * 2); d++)
                        {
                            pack = 0;
                            for (int e = 0; e < (mod * 2); e++)
                            {
                                offset = mux[e];
                                pack |= tempu[index + offset];
                                pack <<= 1;
                            }
                            pack >>= 1;
                            out[produced++] = pack >> 8;
                            out[produced++] = pack & 0xff;
                            index += (mod * 2);
                            consumed += (mod * 2);
                        }
                    }
                }
                else
                {
                    if (code_rate == gr::dvbt2::C1_3)
                    {
                        mux = &mux256s_13[0];
                    }
                    else if (code_rate == gr::dvbt2::C2_5)
                    {
                        mux = &mux256s_25[0];
                    }
                    else
                    {
                        mux = &mux256s[0];
                    }
                    for (int i = 0; i < noutput_items; i += packed_items)
                    {
                        rows = frame_size / mod;
                        const unsigned char *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8;
                        c1 = &tempv[0];
                        c2 = &tempv[rows];
                        c3 = &tempv[rows * 2];
                        c4 = &tempv[rows * 3];
                        c5 = &tempv[rows * 4];
                        c6 = &tempv[rows * 5];
                        c7 = &tempv[rows * 6];
                        c8 = &tempv[rows * 7];
                        for (int k = 0; k < nbch; k++)
                        {
                            tempu[k] = *in++;
                        }
                        for (int t = 0; t < q_val; t++)
                        {
                            for (int s = 0; s < 360; s++)
                            {
                                tempu[nbch + (360 * t) + s] = in[(q_val * s) + t];
                            }
                        }
                        in = in + (q_val * 360);
                        index = 0;
                        for (int col = 0; col < mod; col++)
                        {
                            offset = twist256s[col];
                            for (int row = 0; row < rows; row++)
                            {
                                tempv[offset + (rows * col)] = tempu[index++];
                                offset = (offset + 1) % rows;
                            }
                        }
                        index = 0;
                        for (int j = 0; j < rows; j++)
                        {
                            tempu[index++] = c1[j];
                            tempu[index++] = c2[j];
                            tempu[index++] = c3[j];
                            tempu[index++] = c4[j];
                            tempu[index++] = c5[j];
                            tempu[index++] = c6[j];
                            tempu[index++] = c7[j];
                            tempu[index++] = c8[j];
                        }
                        index = 0;
                        for (int d = 0; d < frame_size / mod; d++)
                        {
                            pack = 0;
                            for (int e = 0; e < mod; e++)
                            {
                                offset = mux[e];
                                pack |= tempu[index + offset];
                                pack <<= 1;
                            }
                            pack >>= 1;
                            out[produced++] = pack & 0xff;
                            index += mod;
                            consumed += mod;
                        }
                    }
                }
                break;
        }

        // Tell runtime system how many input items we consumed on
        // each input stream.
        consume_each (consumed);

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

    const int interleaver_bb_impl::twist16n[8] =
    {
        0, 0, 2, 4, 4, 5, 7, 7
    };

    const int interleaver_bb_impl::twist64n[12] =
    {
        0, 0, 2, 2, 3, 4, 4, 5, 5, 7, 8, 9
    };

    const int interleaver_bb_impl::twist256n[16] =
    {
        0, 2, 2, 2, 2, 3, 7, 15, 16, 20, 22, 22, 27, 27, 28, 32
    };

    const int interleaver_bb_impl::twist16s[8] =
    {
        0, 0, 0, 1, 7, 20, 20, 21
    };

    const int interleaver_bb_impl::twist64s[12] =
    {
        0, 0, 0, 2, 2, 2, 3, 3, 3, 6, 7, 7
    };

    const int interleaver_bb_impl::twist256s[8] =
    {
        0, 0, 0, 1, 7, 20, 20, 21
    };

    const int interleaver_bb_impl::mux16[8] =
    {
        7, 1, 3, 5, 2, 4, 6, 0
    };

    const int interleaver_bb_impl::mux64[12] =
    {
        11, 8, 5, 2, 10, 7, 4, 1, 9, 6, 3, 0
    };

    const int interleaver_bb_impl::mux256[16] =
    {
        15, 1, 13, 3, 10, 7, 9, 11, 4, 6, 8, 5, 12, 2, 14, 0
    };

    const int interleaver_bb_impl::mux16_35[8] =
    {
        0, 2, 3, 6, 4, 1, 7, 5
    };

    const int interleaver_bb_impl::mux16_13[8] =
    {
        1, 6, 5, 2, 3, 4, 0, 7
    };

    const int interleaver_bb_impl::mux16_25[8] =
    {
        3, 5, 6, 4, 2, 1, 7, 0
    };

    const int interleaver_bb_impl::mux64_35[12] =
    {
        4, 6, 0, 5, 8, 10, 2, 1, 7, 3, 11, 9
    };

    const int interleaver_bb_impl::mux64_13[12] =
    {
        2, 5, 1, 6, 0, 3, 4, 7, 8, 9, 10, 11
    };

    const int interleaver_bb_impl::mux64_25[12] =
    {
        1, 2, 4, 5, 0, 6, 3, 8, 7, 10, 9, 11
    };

    const int interleaver_bb_impl::mux256_35[16] =
    {
        4, 6, 0, 2, 3, 14, 12, 10, 7, 5, 8, 1, 15, 9, 11, 13
    };

    const int interleaver_bb_impl::mux256_23[16] =
    {
        3, 15, 1, 7, 4, 11, 5, 0, 12, 2, 9, 14, 13, 6, 8, 10
    };

    const int interleaver_bb_impl::mux256s[8] =
    {
        7, 2, 4, 1, 6, 3, 5, 0
    };

    const int interleaver_bb_impl::mux256s_13[8] =
    {
        1, 2, 3, 5, 0, 4, 6, 7
    };

    const int interleaver_bb_impl::mux256s_25[8] =
    {
        1, 3, 4, 5, 0, 2, 6, 7
    };

  } /* namespace dvbt2 */
} /* namespace gr */

