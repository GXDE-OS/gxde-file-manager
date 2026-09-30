/*
 * OFD 缩略图生成工具
 *
 * 用法: ofd <size> <file>
 *
 * 使用 librofd-ffi 将 OFD 文档第一页渲染为 PNG，并以 base64 编码输出到标准输出。
 * 该工具与 dde-file-thumbnail-tool/video 保持一致的接口，由
 * DThumbnailProvider 通过外部工具回退路径（keyToThumbnailTool）调用。
 */

#include <rofd.h>
#include <cairo.h>

#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QString>
#include <QPainter>

static QByteArray renderOfdThumbnail(const QString &filePath, int size)
{
    QByteArray pngData;

    rofd_error_t *error = nullptr;
    rofd_document_t *doc = nullptr;
    rofd_status_t status = rofd_document_open(filePath.toUtf8().constData(), nullptr, &doc, &error);

    if (status != ROFD_STATUS_OK || !doc) {
        if (error)
            rofd_error_free(error);
        return pngData;
    }

    size_t count = 0;
    error = nullptr;
    status = rofd_document_get_page_count(doc, &count, &error);

    if (status != ROFD_STATUS_OK || count == 0) {
        if (error)
            rofd_error_free(error);
        rofd_document_free(doc);
        return pngData;
    }

    rofd_renderer_options_t rendererOptions;
    rofd_renderer_options_init(&rendererOptions, sizeof(rendererOptions));
    error = nullptr;
    rofd_renderer_t *renderer = nullptr;
    status = rofd_renderer_new(&rendererOptions, &renderer, &error);

    if (status != ROFD_STATUS_OK || !renderer) {
        if (error)
            rofd_error_free(error);
        rofd_document_free(doc);
        return pngData;
    }

    rofd_page_t *page = nullptr;
    error = nullptr;
    status = rofd_document_get_page(doc, 0, &page, &error);

    if (status != ROFD_STATUS_OK || !page) {
        if (error)
            rofd_error_free(error);
        rofd_renderer_free(renderer);
        rofd_document_free(doc);
        return pngData;
    }

    rofd_rect_t size_mm = {0.0, 0.0, 0.0, 0.0};
    error = nullptr;
    rofd_page_get_size_mm(page, &size_mm, &error);
    if (error)
        rofd_error_free(error);

    rofd_render_options_t opts;
    rofd_render_options_init(&opts, sizeof(opts));
    opts.dpi = 254.0;
    if (size > 0 && size_mm.width_mm > 0) {
        const double mmPerPx = 25.4 / opts.dpi;
        const double widthPx = size_mm.width_mm / mmPerPx;
        if (widthPx > 0)
            opts.scale = static_cast<double>(size) / widthPx;
    }

    int32_t width = 0;
    int32_t height = 0;
    error = nullptr;
    status = rofd_renderer_get_pixel_canvas_size(renderer, page, &opts, &width, &height, &error);

    if (status != ROFD_STATUS_OK || width <= 0 || height <= 0) {
        if (error)
            rofd_error_free(error);
        rofd_page_free(page);
        rofd_renderer_free(renderer);
        rofd_document_free(doc);
        return pngData;
    }

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        rofd_page_free(page);
        rofd_renderer_free(renderer);
        rofd_document_free(doc);
        return pngData;
    }

    cairo_t *cr = cairo_create(surface);
    rofd_render_report_t *report = nullptr;
    error = nullptr;
    status = rofd_renderer_render_page_cairo(renderer, page, cr, &opts, &report, &error);

    QImage img;
    if (status == ROFD_STATUS_OK) {
        cairo_surface_flush(surface);
        unsigned char *data = cairo_image_surface_get_data(surface);
        const int stride = cairo_image_surface_get_stride(surface);
        img = QImage(data, width, height, stride, QImage::Format_ARGB32_Premultiplied).copy();
    }

    if (report)
        rofd_render_report_free(report);
    if (error)
        rofd_error_free(error);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    rofd_page_free(page);
    rofd_renderer_free(renderer);
    rofd_document_free(doc);

    if (img.isNull())
        return pngData;

    // OFD 页面应为白底；对透明区域做白底合成，避免 PNG 透明黑边
    if (img.hasAlphaChannel()) {
        QImage opaque(img.size(), QImage::Format_RGB32);
        opaque.fill(Qt::white);
        QPainter painter(&opaque);
        painter.drawImage(0, 0, img);
        img = opaque;
    } else {
        img = img.convertToFormat(QImage::Format_RGB32);
    }

    if (size > 0 && (img.width() > size || img.height() > size))
        img = img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QBuffer buffer(&pngData);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");

    return pngData;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        return -1;

    const int size = QString(argv[1]).toInt();
    const QString filePath = QString::fromLocal8Bit(argv[2]);

    const QByteArray png = renderOfdThumbnail(filePath, size);

    if (png.isEmpty())
        return -1;

    const QByteArray base64 = png.toBase64();
    printf("%s", base64.constData());
    fflush(stdout);

    return 0;
}
