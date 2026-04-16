#pragma once

#include <string>
#include <vector>

// Поддерживаемые форматы изображений
enum class ImageFormat {
    JPEG,
    PNG,
    BMP,
    TGA,
    WEBP,
    TIFF,
    UNKNOWN
};

// Информация об изображении
struct ImageInfo {
    std::string filepath;
    int         width    = 0;
    int         height   = 0;
    int         channels = 0;
    ImageFormat format   = ImageFormat::UNKNOWN;
    bool        valid    = false;
};

// Результат конвертации
struct ConversionResult {
    bool        success = false;
    std::string message;
    std::string outputPath;
};

// Статический класс для работы с изображениями
class ImageConverter {
public:
    static ImageFormat               detectFormat(const std::string& filepath);
    static std::string               formatToString(ImageFormat fmt);
    static std::string               formatToExtension(ImageFormat fmt);
    static std::string               formatDescription(ImageFormat fmt);
    static std::vector<ImageFormat>  supportedFormats();

    static ImageInfo        getInfo(const std::string& filepath);
    static ConversionResult convert(const std::string& inputPath,
                                    ImageFormat         targetFormat,
                                    int                 jpegQuality   = 90,
                                    int                 webpQuality   = 90);

private:
    static std::string getExtension(const std::string& filepath);
    static std::string getStem(const std::string& filepath);
    static std::string getDirectory(const std::string& filepath);
    static std::string toLowerCase(const std::string& s);
};
