// stb_image должен быть скомпилирован ровно в одном .cpp файле
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"

#pragma GCC diagnostic pop

#include "ImageConverter.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <cstring>
#include <fstream>
#include <vector>

using namespace std;

#ifndef PLATFORM_WINDOWS
  #include <webp/encode.h>
  #include <webp/decode.h>
  #include <tiffio.h>
  #define HAVE_WEBP
  #define HAVE_TIFF
#endif

string ImageConverter::toLowerCase(const string& s) {
    string r = s;
    transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return tolower(c); });
    return r;
}

string ImageConverter::getExtension(const string& filepath) {
    const auto pos = filepath.rfind('.');
    return (pos == string::npos) ? "" : toLowerCase(filepath.substr(pos));
}

string ImageConverter::getStem(const string& filepath) {
    const auto sep = filepath.find_last_of("/\\");
    const string name = (sep == string::npos) ? filepath : filepath.substr(sep + 1);
    const auto dot = name.rfind('.');
    return (dot == string::npos) ? name : name.substr(0, dot);
}

string ImageConverter::getDirectory(const string& filepath) {
    const auto sep = filepath.find_last_of("/\\");
    return (sep == string::npos) ? "" : filepath.substr(0, sep);
}

ImageFormat ImageConverter::detectFormat(const string& filepath) {
    const string ext = getExtension(filepath);
    if (ext == ".jpg" || ext == ".jpeg") return ImageFormat::JPEG;
    if (ext == ".png")                   return ImageFormat::PNG;
    if (ext == ".bmp")                   return ImageFormat::BMP;
    if (ext == ".tga")                   return ImageFormat::TGA;
    if (ext == ".webp")                  return ImageFormat::WEBP;
    if (ext == ".tif" || ext == ".tiff") return ImageFormat::TIFF;
    return ImageFormat::UNKNOWN;
}

string ImageConverter::formatToString(ImageFormat fmt) {
    switch (fmt) {
        case ImageFormat::JPEG: return "JPEG";
        case ImageFormat::PNG:  return "PNG";
        case ImageFormat::BMP:  return "BMP";
        case ImageFormat::TGA:  return "TGA";
        case ImageFormat::WEBP: return "WebP";
        case ImageFormat::TIFF: return "TIFF";
        default:                return "Unknown";
    }
}

string ImageConverter::formatToExtension(ImageFormat fmt) {
    switch (fmt) {
        case ImageFormat::JPEG: return ".jpg";
        case ImageFormat::PNG:  return ".png";
        case ImageFormat::BMP:  return ".bmp";
        case ImageFormat::TGA:  return ".tga";
        case ImageFormat::WEBP: return ".webp";
        case ImageFormat::TIFF: return ".tiff";
        default:                return "";
    }
}

string ImageConverter::formatDescription(ImageFormat fmt) {
    switch (fmt) {
        case ImageFormat::JPEG: return "Lossy — компактный, идеален для фотографий";
        case ImageFormat::PNG:  return "Lossless — без потерь, поддерживает прозрачность";
        case ImageFormat::BMP:  return "Uncompressed — без сжатия, максимальное качество";
        case ImageFormat::TGA:  return "Targa — профессиональная графика и игровые движки";
        case ImageFormat::WEBP: return "Google WebP — современный веб-формат, малый размер";
        case ImageFormat::TIFF: return "TIFF — полиграфия и профессиональная обработка";
        default:                return "";
    }
}

vector<ImageFormat> ImageConverter::supportedFormats() {
    vector<ImageFormat> fmts = { ImageFormat::JPEG, ImageFormat::PNG,
                                      ImageFormat::BMP,  ImageFormat::TGA };
#ifdef HAVE_WEBP
    fmts.push_back(ImageFormat::WEBP);
#endif
#ifdef HAVE_TIFF
    fmts.push_back(ImageFormat::TIFF);
#endif
    return fmts;
}

static unsigned char* loadAny(const string& path, int& w, int& h, int& channels) {
    const string ext = [&]{
        string e = path.substr(path.rfind('.') < path.size() ? path.rfind('.') : path.size());
        transform(e.begin(), e.end(), e.begin(),
                       [](unsigned char c){ return tolower(c); });
        return e;
    }();

#ifdef HAVE_WEBP
    if (ext == ".webp") {
        ifstream f(path, ios::binary | ios::ate);
        if (!f) return nullptr;
        const streamsize fsize = f.tellg();
        f.seekg(0);
        vector<uint8_t> buf(static_cast<size_t>(fsize));
        if (!f.read(reinterpret_cast<char*>(buf.data()), fsize))
            return nullptr;

        WebPBitstreamFeatures feat;
        if (WebPGetFeatures(buf.data(), buf.size(), &feat) != VP8_STATUS_OK) return nullptr;
        w = feat.width; h = feat.height;
        channels = feat.has_alpha ? 4 : 3;

        return feat.has_alpha
            ? WebPDecodeRGBA(buf.data(), buf.size(), &w, &h)
            : WebPDecodeRGB(buf.data(), buf.size(), &w, &h);
    }
#endif

#ifdef HAVE_TIFF
    if (ext == ".tif" || ext == ".tiff") {
        TIFFSetWarningHandler(nullptr);
        TIFF* tif = TIFFOpen(path.c_str(), "r");
        if (!tif) return nullptr;

        uint32_t tw = 0, th = 0;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &tw);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &th);
        w = static_cast<int>(tw);
        h = static_cast<int>(th);
        channels = 4;

        // _TIFFmalloc — функция из библиотеки libtiff для выделения памяти под растр
        auto* raster = static_cast<uint32_t*>(_TIFFmalloc(tw * th * sizeof(uint32_t)));
        if (!raster || !TIFFReadRGBAImageOriented(tif, tw, th, raster, ORIENTATION_TOPLEFT, 0)) {
            if (raster) _TIFFfree(raster);
            TIFFClose(tif);
            return nullptr;
        }
        TIFFClose(tif);

        auto* out = new unsigned char[tw * th * 4];
        for (uint32_t i = 0; i < tw * th; ++i) {
            out[i * 4 + 0] = TIFFGetR(raster[i]);
            out[i * 4 + 1] = TIFFGetG(raster[i]);
            out[i * 4 + 2] = TIFFGetB(raster[i]);
            out[i * 4 + 3] = TIFFGetA(raster[i]);
        }
        _TIFFfree(raster);
        return out;
    }
#endif

    return stbi_load(path.c_str(), &w, &h, &channels, 0);
}

static void freePixels(unsigned char* px, ImageFormat srcFmt) {
#ifdef HAVE_WEBP
    if (srcFmt == ImageFormat::WEBP) { WebPFree(px); return; }
#endif
#ifdef HAVE_TIFF
    if (srcFmt == ImageFormat::TIFF) { delete[] px; return; }
#endif
    (void)srcFmt;
    stbi_image_free(px);
}

ImageInfo ImageConverter::getInfo(const string& filepath) {
    ImageInfo info;
    info.filepath = filepath;
    info.format   = detectFormat(filepath);

    const bool needsFullLoad = (info.format == ImageFormat::WEBP || info.format == ImageFormat::TIFF);
    if (needsFullLoad) {
        int w = 0, h = 0, c = 0;
        unsigned char* px = loadAny(filepath, w, h, c);
        if (px) {
            info.width = w; info.height = h; info.channels = c;
            info.valid = true;
            freePixels(px, info.format);
        }
    } else {
        info.valid = (stbi_info(filepath.c_str(), &info.width, &info.height, &info.channels) == 1)
                     && (info.width > 0);
    }
    return info;
}

ConversionResult ImageConverter::convert(const string& inputPath,
                                         ImageFormat    targetFormat,
                                         int            jpegQuality,
                                         [[maybe_unused]] int webpQuality) {
    ConversionResult result;

    const ImageFormat srcFmt = detectFormat(inputPath);

    int w = 0, h = 0, channels = 0;
    unsigned char* rawPixels = loadAny(inputPath, w, h, channels);

    if (!rawPixels) {
        result.message = string("Не удалось загрузить изображение: ")
                         + stbi_failure_reason();
        return result;
    }

    // RAII-охранник для автоматического освобождения пикселей
    struct PixelGuard {
        unsigned char* px;
        ImageFormat    fmt;
        ~PixelGuard() { freePixels(px, fmt); }
    } guard{ rawPixels, srcFmt };

    const string dir     = getDirectory(inputPath);
    const string stem    = getStem(inputPath);
    const string ext     = formatToExtension(targetFormat);
    const string outPath = (dir.empty() ? "" : dir + "/") + stem + ext;

    int ok = 0;

    switch (targetFormat) {
        case ImageFormat::JPEG:
            ok = stbi_write_jpg(outPath.c_str(), w, h, channels, rawPixels, jpegQuality);
            break;

        case ImageFormat::PNG:
            ok = stbi_write_png(outPath.c_str(), w, h, channels, rawPixels, w * channels);
            break;

        case ImageFormat::BMP:
            ok = stbi_write_bmp(outPath.c_str(), w, h, channels, rawPixels);
            break;

        case ImageFormat::TGA:
            ok = stbi_write_tga(outPath.c_str(), w, h, channels, rawPixels);
            break;

#ifdef HAVE_WEBP
        case ImageFormat::WEBP: {
            uint8_t* output = nullptr;
            size_t   size   = 0;
            if (channels == 4)
                size = WebPEncodeRGBA(rawPixels, w, h, w * 4, static_cast<float>(webpQuality), &output);
            else
                size = WebPEncodeRGB(rawPixels,  w, h, w * 3, static_cast<float>(webpQuality), &output);

            if (size > 0 && output) {
                ofstream f(outPath, ios::binary);
                if (f) {
                    f.write(reinterpret_cast<const char*>(output), static_cast<streamsize>(size));
                    ok = f.good() ? 1 : 0;
                }
                WebPFree(output);
            }
            break;
        }
#endif

#ifdef HAVE_TIFF
        case ImageFormat::TIFF: {
            TIFFSetWarningHandler(nullptr);
            TIFF* tif = TIFFOpen(outPath.c_str(), "w");
            if (tif) {
                TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,      w);
                TIFFSetField(tif, TIFFTAG_IMAGELENGTH,     h);
                TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, channels);
                TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,   8);
                TIFFSetField(tif, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);
                TIFFSetField(tif, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
                TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,
                             channels >= 3 ? PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK);
                TIFFSetField(tif, TIFFTAG_COMPRESSION,     COMPRESSION_LZW);
                TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,    TIFFDefaultStripSize(tif, w * channels));

                for (int row = 0; row < h; ++row)
                    TIFFWriteScanline(tif, rawPixels + row * w * channels, row, 0);

                TIFFClose(tif);
                ok = 1;
            }
            break;
        }
#endif

        default:
            result.message = "Неподдерживаемый целевой формат.";
            return result;
    }

    if (ok == 0) {
        result.message = "Не удалось записать файл. Проверьте права доступа к папке.";
        return result;
    }

    result.success    = true;
    result.message    = "Конвертация выполнена успешно.";
    result.outputPath = outPath;
    return result;
}
