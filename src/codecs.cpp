/**
 *Implementations of the Codec interface
 *
 * Written by m^2.
 * You can consider the code to be public domain.
 * If your country doesn't recognize author's right to relieve themselves of copyright,
 * you can use it under the terms of WTFPL version 2.0 or later.
 */

#include "codecs.hpp"
#include "simple_codecs.hpp"
#include "tools.hpp"

#include <algorithm>

using namespace std;

//////////////////////////////////////////////////////
// Codecs that use some of the abstract classes
// Here they have functions used by their constructors
//////////////////////////////////////////////////////

#ifdef FSBENCH_USE_7Z
#include "7z/7z.hpp"
#endif//FSBENCH_USE_7Z
#ifdef FSBENCH_USE_BCL
#include "bcl.hpp"
#endif//FSBENCH_USE_BCL
#ifdef FSBENCH_USE_ECRYPT
#include "ecrypt/fsbench_ecrypt.hpp"
#endif//FSBENCH_USE_ECRYPT
#ifdef FSBENCH_USE_CRYPTOPP
#include "cryptopp/cryptopp.hpp"
#endif//FSBENCH_USE_CRYPTOPP
#ifdef FSBENCH_USE_GIPFELI
#include "fsbench_gipfeli.hpp"
#endif//FSBENCH_USE_GIPFELI
#ifdef FSBENCH_USE_MINIZ
#include "miniz.hpp"
#endif//FSBENCH_USE_MINIZ
#ifdef FSBENCH_USE_SCZ
#include "scz.hpp"
#endif// FSBENCH_USE_SCZ
#if defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#include "fsbench_SHA3.hpp"
#endif// defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#ifdef FSBENCH_USE_TINF
#include "tinf/tinf.hpp"
#endif//FSBENCH_USE_TINF
/////////////////////////////////
// Global variables 
// and helpers to manipulate them
/////////////////////////////////

// Just looks up a codec in CODECS
Codec* raw_find_codec(const string& name)
{
    for (list<Codec*>::iterator it = CODECS.begin(); it != CODECS.end(); ++it)
    {
        if (!case_insensitive_compare((*it)->name, name))
        {
            return *it;
        }
    }
    return 0;
}
;

// Looks up a codec in CODECS.
// When it can't be found, but is a combination of other codecs,
// it adds it to CODECS
// The addition thing is here because it is needed to create a new codec
// But who is supposed to dispose it?
// Not the caller because it can't tell whether the pointer shows something created now or before
// The could be worked around, but I think that only in some ugly ways
// Not we, because the codec wouldn't live long enough
// This has some funny side effects like codec1/codec2/codec3 that works iff you did codec2/codec3 before
Codec* find_codec(const string& name)
{
    Codec* ret = raw_find_codec(name);
    if (!ret)
    {
        // maybe it's a pipeline of existing codecs?
        string::size_type pos = name.find('+');
        if (pos != string::npos)
        {
            string first_name = name.substr(0, pos);
            string second_name = name.substr(pos + 1);
            Codec* first = find_codec(first_name);
            Codec* second = find_codec(second_name);
            if (first && second)
            {
                // good, it's a pipeline of existing codecs
                PipelineCodec* codec = new PipelineCodec(*first, *second);
                CODECS.push_back(codec);
                ret = codec;
            }
        }
        else
        {
            // maybe it's a combination of some existing encoder and decoder?
            string::size_type pos = name.find('/');
            if (pos != string::npos)
            {
                string encoder_name = name.substr(0, pos);
                string decoder_name = name.substr(pos + 1);
                Codec* encoder = raw_find_codec(encoder_name);
                Codec* decoder = raw_find_codec(decoder_name);
                if (encoder && decoder)
                {
                    // good, it's a combination of existing codecs
                    CombinationCodec* codec = new CombinationCodec(*encoder, *decoder);
                    CODECS.push_back(codec);
                    ret = codec;
                }
            }
        }
    }
    return ret;
}

#ifdef FSBENCH_USE_FASTLZ
static const CodecArgs fastlz_args[] =
{
    CodecArgs("1", fastlz1_c, fastlz_d),
    CodecArgs("2", fastlz2_c, fastlz_d)};
#endif
#ifdef FSBENCH_USE_LZF
static const CodecArgs LZF_args[] =
{
    CodecArgs("", LZF_c, LZF_d),
    CodecArgs("very", LZF_very_c, LZF_d),
    CodecArgs("ultra", LZF_ultra_c, LZF_d)
};
#endif
#ifdef FSBENCH_USE_RLE64
static const CodecArgs RLE64_args[] =
{
    CodecArgs("8", RLE8_c, RLE8_d),
    CodecArgs("16", RLE16_c, RLE16_d),
    CodecArgs("32", RLE32_c, RLE32_d),
    CodecArgs("64", RLE64_c, RLE64_d)
};
#endif

Codec * codecs[] =
            {
#ifdef FSBENCH_USE_FASTLZ
              new MultifunctionCodec(
                      "fastlz", _FASTLZ_VERSION,
                      fastlz_args,
                      2,
                      "1",
                      fastlz_m),
#endif
#ifdef FSBENCH_USE_LZF
              new MultifunctionCodec(
                      "LZF", _LZF_VERSION,
                      LZF_args,
                      3,
                      "",
                      no_blowup),
#endif
#ifdef FSBENCH_USE_RLE64
              new MultifunctionCodec(
                      "RLE64", _RLE64_VERSION,
                      RLE64_args,
                      4,
                      "64"),
#endif
#ifdef FSBENCH_USE_7Z
              new CodecWithIntModes("7z-deflate", _7z_VERSION, FsBench7z::deflate, FsBench7z::inflate, 1, 9, "5", no_blowup),
              new CodecWithIntModes("7z-deflate64", _7z_VERSION, FsBench7z::deflate64, FsBench7z::inflate64, 1, 9, "5", no_blowup),
#endif
#ifdef FSBENCH_USE_BLOSC
              new CodecWithIntModes("blosc", _BLOSC_VERSION, blosc_c, blosc_d, 1, 9, "5", no_blowup),
#endif
#ifdef FSBENCH_USE_BZ2
              new CodecWithIntModes("bzip2", _BZ2_VERSION, bz2_c, bz2_d, 1, 9, "9", no_blowup),
#endif
#ifdef FSBENCH_USE_CRYPTOPP
              new CodecWithIntModes("cryptopp-deflate", _CRYPTOPP_VERSION, FsBenchCryptoPP::deflate, FsBenchCryptoPP::inflate, 1, 9, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_LODEPNG
              new CodecWithIntModes("lodepng", _LODEPNG_VERSION, lodepng_c, lodepng_d, 64, 32768, "32768", no_blowup),
#endif
#ifdef FSBENCH_USE_LZG 
              new CodecWithIntModes("lzg", _LZG_VERSION, LZG_c, LZG_d, 1, 9, "5", LZG_m),
#endif
#ifdef FSBENCH_USE_LZX_COMPRESS
              new CodecWithIntModes("lzx_compress", _LZX_VERSION, LZX_c, 0, 15, 21, "21"),
#endif
#ifdef FSBENCH_USE_MINIZ
              new CodecWithIntModes("miniz", _MINIZ_VERSION, miniz_c, miniz_d, 1, 10, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_ZLIB
              new CodecWithIntModes("zlib", _ZLIB_VERSION, zlib_c, zlib_d, 1, 9, "6", no_blowup),
#endif
#ifdef FSBENCH_USE_ZOPFLI
              // I say no_blowup because while zopfli doesn't control its overhead,
              // it uses auxiliary arrays, so won't write outside of the regular ones
              new CodecWithIntModes("zopfli", _ZOPFLI_VERSION, zopfli_c, 0, 1, 1000000, "15", no_blowup),
#endif
#if 0
#ifdef FSBENCH_USE_7Z
              new Codec("7z-lzx", _7z_VERSION, 0, FsBench7z::unlzx, no_blowup),
#endif
#endif
#ifdef FSBENCH_USE_BCL
              new Codec("bcl-huffman", _BCL_VERSION, FsBenchBCL::huffman, FsBenchBCL::unhuffman, FsBenchBCL::huffman_maxsize),
              new Codec("bcl-lz", _BCL_VERSION, FsBenchBCL::lz, FsBenchBCL::unlz, FsBenchBCL::lz_maxsize),
              new Codec("bcl-rle", _BCL_VERSION, FsBenchBCL::rle, FsBenchBCL::unrle, FsBenchBCL::rle_maxsize),
              new FsBenchBCL::LZFast(),
#endif
#ifdef FSBENCH_USE_DOBOZ
              new Codec("Doboz", _DOBOZ_VERSION, Doboz_c, Doboz_d, no_blowup),
#endif
#ifdef FSBENCH_USE_GIPFELI
              new Codec("gipfeli", _GIPFELI_VERSION, FsBenchGipfeli::compress, FsBenchGipfeli::decompress, FsBenchGipfeli::max_size),
#endif
#ifdef FSBENCH_USE_HALIBUT
              new Codec("Halibut-deflate", _HALIBUT_VERSION, halibut_c, halibut_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZ4
              new Codec("LZ4", _LZ4_VERSION, LZ4_c, LZ4_d, no_blowup),
              new Codec("LZ4bz", _LZ4_VERSION, LZ4bz_c, LZ4bz_d, no_blowup), // LZ4bz_m is not needed as these functions don't expand the input anyway
              new Codec("LZ4hc", _LZ4_VERSION, LZ4hc_c, LZ4_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZFX
              new Codec("LZFX", _LZFX_VERSION, LZFX_c, LZFX_d),
#endif
#ifdef FSBENCH_USE_ZFS
              new Codec("lzjb", _ZFS_VERSION, LZJB_c, LZJB_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZMAT
              new Codec("lzmat", _LZMAT_VERSION, lzmat_c, lzmat_d, no_blowup),
#endif
#ifdef FSBENCH_USE_LZV1
              new BufferedCodec("LZV1", _LZV1_VERSION, LZV1_c, LZV1_d, no_blowup, sizeof(ush)*16384, sizeof(ush)),
#endif
#ifdef FSBENCH_USE_LZWC
              new Codec("LZWC", _LZWC_VERSION, LZWC_c, LZWC_d, no_blowup),
#endif
#ifdef FSBENCH_USE_MMINI
              new BufferedCodec("mmini_huffman", _MMINI_VERSION, mmini_huffman_c, mmini_huffman_d, no_blowup, MMINI_HUFFHEAP_SIZE),
              new Codec("mmini_lzl", _MMINI_VERSION, mmini_lzl_c, mmini_lzl_d, no_blowup),
#endif
#ifdef FSBENCH_USE_QUICKLZZIP
              new Codec("QuickLZ-zip", _QLZZIP_VERSION, qlzzip_c, 0),
#endif
#ifdef FSBENCH_USE_SHRINKER
              new Codec("Shrinker", _SHRINKER_VERSION, Shrinker_c, Shrinker_d, Shrinker_m),
#endif
#ifdef FSBENCH_USE_SNAPPY
              new Codec("Snappy", _SNAPPY_VERSION, snappy_c, snappy_d, snappy_m),
#endif
#ifdef FSBENCH_USE_TINF
              new Codec("tinf", _TINF_VERSION, 0, FsBenchTinf::inflate, no_blowup),
#endif
#ifdef FSBENCH_USE_NRV
              new Nrv("b"),
              new Nrv("d"),
              new Nrv("e"),
#endif
#ifdef FSBENCH_USE_BLZ
              new BriefLZ(),
#endif
#ifdef FSBENCH_USE_LZHAM
              new LZHAM(),
#endif
#ifdef FSBENCH_USE_LZO
              new LZO(),
#endif
#ifdef FSBENCH_USE_LZSS_IM
              new LZSS_IM(),
#endif
#ifdef FSBENCH_USE_QUICKLZ
              new QuickLZ(),
#endif
#ifdef FSBENCH_USE_SCZ
              new Scz(),
#endif
#ifdef FSBENCH_USE_TOR
              new Tor(),
#endif
#ifdef FSBENCH_USE_YAPPY
              new Yappy,
#endif
#ifdef FSBENCH_USE_BLAKE2
              new Checksum<64>("Blake2b", _BLAKE2_VERSION, fsbench_blake2b),
              new Checksum<64>("Blake2bp", _BLAKE2_VERSION, fsbench_blake2bp),
              new Checksum<32>("Blake2s", _BLAKE2_VERSION, fsbench_blake2s),
              new Checksum<32>("Blake2sp", _BLAKE2_VERSION, fsbench_blake2sp),
#endif
#ifdef FSBENCH_USE_CITYHASH
              new Checksum< sizeof(uint64_t)>("CityHash64", _CITYHASH_VERSION, CityHash64),
              new Checksum<2*sizeof(uint64_t)>("CityHash128", _CITYHASH_VERSION, CityHash128),
#endif
#ifdef FSBENCH_USE_CRAPWOW
              new Checksum<sizeof(unsigned int)>("CrapWow", _CRAPWOW_VERSION, CrapWow),
#endif
#ifdef FSBENCH_USE_CRYPTOPP
              new Checksum<sizeof(uint32_t)>("cryptopp-adler32", _CRYPTOPP_VERSION, FsBenchCryptoPP::adler32),
              new Checksum<sizeof(uint32_t)>("cryptopp-crc32", _CRYPTOPP_VERSION, FsBenchCryptoPP::crc),
              new Checksum<128/CHAR_BIT> ("cryptopp-md5", _CRYPTOPP_VERSION, FsBenchCryptoPP::md5),
              new Checksum<224/CHAR_BIT> ("cryptopp-sha224", _CRYPTOPP_VERSION, FsBenchCryptoPP::sha224),
              new Checksum<256/CHAR_BIT> ("cryptopp-sha256", _CRYPTOPP_VERSION, FsBenchCryptoPP::sha256),
              new Checksum<384/CHAR_BIT> ("cryptopp-sha384", _CRYPTOPP_VERSION, FsBenchCryptoPP::sha384),
              new Checksum<512/CHAR_BIT> ("cryptopp-sha512", _CRYPTOPP_VERSION, FsBenchCryptoPP::sha512),
#endif
#ifdef FSBENCH_USE_MURMUR
              new Checksum<8>("SipHash24", _SIPHASH_VERSION, siphash),
#endif
#ifdef FSBENCH_USE_SIPHASH
              new Checksum< sizeof(uint32_t)>("murmur3_x86_32", _MURMUR_VERSION, murmur_x86_32),
#endif
#ifdef FSBENCH_USE_SPOOKY
              new Checksum<2*sizeof(uint64_t)>("SpookyHash", _SPOOKY_VERSION, spooky),
#endif
#ifdef FSBENCH_USE_SANMAYCE_FNV
              new Checksum<sizeof(uint32_t)>("FNV1a-Jesteress", _SANMAYCE_VERSION, fnv1_jesteress),
              new Checksum<sizeof(uint32_t)>("FNV1a-Mantis", _SANMAYCE_VERSION, fnv1_mantis),
              new Checksum<sizeof(uint32_t)>("FNV1a-Meiyan", _SANMAYCE_VERSION, fnv1_meiyan),
              new Checksum<sizeof(uint64_t)>("FNV1a-Tesla", _SANMAYCE_VERSION, fnv1_tesla),
              new Checksum<sizeof(uint64_t)>("FNV1a-Tesla3", _SANMAYCE_VERSION, fnv1_tesla3),
              new Checksum<sizeof(uint64_t)>("FNV1a-Yoshimura", _SANMAYCE_VERSION, fnv1_yoshimura),
#endif
#if defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3) || defined(FSBENCH_USE_SHA3_RND3_GROESTL)
#   define QUOTE(x) #x
#   define SHA3_CHECKSUM(name, digest_size, version)\
    new Checksum<digest_size/CHAR_BIT>(QUOTE(name##digest_size),              version,            FsBenchSHA3::name::fsbench_hash_##digest_size),
#   ifdef FSBENCH_USE_SHA3_RND1
#   define EDON_R(digest_size)\
    new Checksum<digest_size/CHAR_BIT>("Edon-R" QUOTE(digest_size),           _EDON_R_VERSION,    FsBenchSHA3::Edon_R::fsbench_hash_##digest_size),
              EDON_R(224)
              EDON_R(256)
              EDON_R(384)
              EDON_R(512)
              SHA3_CHECKSUM(SWIFFTX, 224, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 256, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(SWIFFTX, 512, _SHA3_RND1_VERSION)
#   endif
#   ifdef FSBENCH_USE_SHA3_RND2
#   define BMW2(digest_size)\
    new Checksum<digest_size/CHAR_BIT>("BlueMidnightWish" QUOTE(digest_size), _SHA3_RND2_VERSION, FsBenchSHA3::BlueMidnightWish2::fsbench_hash_##digest_size),
              BMW2(224)
              BMW2(256)
              SHA3_CHECKSUM(BlueMidnightWish, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(BlueMidnightWish, 512, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 224, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 256, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 384, _SHA3_RND1_VERSION)
              SHA3_CHECKSUM(CubeHash, 512, _SHA3_RND1_VERSION)
#   endif
#   ifdef FSBENCH_USE_SHA3_RND3
              SHA3_CHECKSUM(Blake, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Blake, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Keccak, 224, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 256, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 384, _KECCAK_VERSION)
              SHA3_CHECKSUM(Keccak, 512, _KECCAK_VERSION)
              SHA3_CHECKSUM(JH, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(JH, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 512, _SHA3_VERSION)
              SHA3_CHECKSUM(Skein, 1024, _SHA3_VERSION)
#   endif
#   ifdef FSBENCH_USE_SHA3_RND3_GROESTL
              SHA3_CHECKSUM(Groestl, 224, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 256, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 384, _SHA3_VERSION)
              SHA3_CHECKSUM(Groestl, 512, _SHA3_VERSION)
#   endif
#endif // defined(FSBENCH_USE_SHA3_RND1) || defined(FSBENCH_USE_SHA3_RND2) || defined(FSBENCH_USE_SHA3_RND3)
#ifdef FSBENCH_USE_XXHASH
              new Checksum<sizeof(unsigned int)>("xxhash", _XXHASH_VERSION, xxhash),
              new Checksum<4*sizeof(uint64_t)> ("xxhash256", _XXHASH_VERSION, xxhash_256),
#endif
#ifdef FSBENCH_USE_ZFS
              new Checksum<4*sizeof(uint64_t)>("fletcher2", _ZFS_VERSION, fletcher2),
              new Checksum<4*sizeof(uint64_t)>("fletcher4", _ZFS_VERSION, fletcher4),
#endif
#ifdef FSBENCH_USE_ECRYPT
              new FsBenchECrypt::AES128Bernstein(),
              new FsBenchECrypt::AES256Hongjun(),
              new FsBenchECrypt::ChaCha(),
              new FsBenchECrypt::HC128(),
              new FsBenchECrypt::HC256(),
              new FsBenchECrypt::Lex(),
              new FsBenchECrypt::Rabbit(),
              new FsBenchECrypt::RC4(),
              new FsBenchECrypt::Salsa20_8(),
              new FsBenchECrypt::Salsa20_12(),
              new FsBenchECrypt::Salsa20(),
              new FsBenchECrypt::Snow2(),
              new FsBenchECrypt::Sosemanuk(),
              new FsBenchECrypt::Trivium(),
#endif
              new Codec("nop", "0", nop_c, nop_d, no_blowup) };

list<Codec*> CODECS = list<Codec*>(codecs, codecs + sizeof(codecs) / sizeof(Codec*));

list<CodecWithParams> createParamsList(list<pair<Codec*, const string> > lst)
{
    list<CodecWithParams> ret;
    for (list<pair<Codec*, const string> >::iterator it = lst.begin(); it != lst.end(); ++it)
    {
        if (it->first)
            ret.push_back(CodecWithParams(*(it->first), it->second));
    }
    return ret;
}
#define MKLIST(name, array) list<CodecWithParams> (name) = createParamsList(list< pair<Codec*,const string> >((array), (array) + sizeof((array)) / sizeof(pair<Codec*,const string>)))

pair<Codec*, string> default_codecs[] =
    { make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(raw_find_codec("Snappy"), "") };
MKLIST(DEFAULT_CODECS, default_codecs);

pair<Codec*, string> fast_codecs[] =
    { make_pair(raw_find_codec("bcl-rle"), ""),
      make_pair(raw_find_codec("blosc"), ""),
      make_pair(raw_find_codec("fastlz"), ""),
      make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZF"), "very"),
      make_pair(raw_find_codec("LZJB"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(raw_find_codec("RLE64"), ""),
      make_pair(raw_find_codec("Shrinker"), ""),
      make_pair(raw_find_codec("Snappy"), "") };
MKLIST(FAST_CODECS, fast_codecs);

pair<Codec*, string> all_codecs[] =
    { make_pair(raw_find_codec("7z-deflate"), ""),
      make_pair(raw_find_codec("7z-deflate64"), ""),
      make_pair(raw_find_codec("bcl-huffman"), ""),
      make_pair(raw_find_codec("bcl-lz"), ""),
      make_pair(raw_find_codec("bcl-lzfast"), ""),
      make_pair(raw_find_codec("bcl-rle"), ""),
      make_pair(raw_find_codec("blosc"), ""),
      make_pair(raw_find_codec("BriefLZ"), ""),
      make_pair(raw_find_codec("bzip2"), ""),
      make_pair(raw_find_codec("cryptopp-deflate"), ""),
      make_pair(raw_find_codec("Doboz"), ""),
      make_pair(raw_find_codec("fastlz"), ""),
      make_pair(raw_find_codec("gipfeli"), ""),
      make_pair(raw_find_codec("halibut-deflate"), ""),
      make_pair(raw_find_codec("lodepng"), ""),
      make_pair(raw_find_codec("LZ4"), ""),
      make_pair(raw_find_codec("LZ4bz"), ""),
      make_pair(raw_find_codec("LZ4hc"), ""),
      make_pair(raw_find_codec("LZF"), ""),
      make_pair(raw_find_codec("LZFX"), ""),
      make_pair(raw_find_codec("lzg"), ""),
      make_pair(raw_find_codec("LZHAM"), ""),
      make_pair(raw_find_codec("LZJB"), ""),
      make_pair(raw_find_codec("lzmat"), ""),
      make_pair(raw_find_codec("LZO"), ""),
      make_pair(raw_find_codec("LZSS-IM"), ""),
      make_pair(raw_find_codec("lzv1"), ""),
      make_pair(find_codec("lzx_compress/nop"), ""),
      make_pair(raw_find_codec("miniz"), ""),
      make_pair(raw_find_codec("nrv2b"), ""),
      make_pair(raw_find_codec("nrv2d"), ""),
      make_pair(raw_find_codec("nrv2e"), ""),
      make_pair(raw_find_codec("QuickLZ"), ""),
      make_pair(raw_find_codec("QuickLZ-zip/zlib"), ""),
      make_pair(raw_find_codec("RLE64"), ""),
      make_pair(raw_find_codec("scz"), ""),
      make_pair(raw_find_codec("Shrinker"), ""),
      make_pair(raw_find_codec("siphash"), ""),
      make_pair(raw_find_codec("Snappy"), ""),
      make_pair(find_codec("zlib/tinf"), ""),
      make_pair(raw_find_codec("Tornado"), ""),
      make_pair(raw_find_codec("Yappy"), ""),
      make_pair(raw_find_codec("zlib"), "")
//note: nop is not included
        };
MKLIST(ALL_CODECS, all_codecs);