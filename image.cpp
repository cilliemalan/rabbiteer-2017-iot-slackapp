#include "pch.h"
#include "image.h"

struct read_data
{
    read_data(size_t ppos, const void* sourcedata, size_t buffersize)
        : ppos(ppos),
          sourcedata(sourcedata),
          buffersize(buffersize)
    {
    }

    size_t read(void* destinationbuffer, int amount)
    {
        auto src = reinterpret_cast<const unsigned char*>(sourcedata);
        auto dst = reinterpret_cast<unsigned char*>(destinationbuffer);

        auto remain = buffersize - ppos;
        auto willread = remain < amount ? remain : amount;

        auto err = memcpy_s(dst, willread, src + ppos, willread);
        if (err) throw std::system_error(err, std::system_category());
        
        ppos += willread;
        return willread;
    }

    size_t ppos;
    const void* sourcedata;
    const size_t buffersize;
};

int gifreadfunc(GifFileType *handle, GifByteType *destinationbuffer, int amount)
{
    auto rdata = reinterpret_cast<read_data*>(handle->UserData);
    return static_cast<int>(rdata->read(destinationbuffer, amount));
}

void png_error(png_structp png, png_const_charp msg)
{
    throw std::runtime_error(msg);
}

void png_warning(png_structp png, png_const_charp msg)
{
}

void png_write(png_structp png, png_bytep buffer, png_size_t amt)
{
    auto ioptr = png_get_io_ptr(png);
    auto& obuffer = *reinterpret_cast<std::vector<unsigned char>*>(ioptr);

    obuffer.insert(obuffer.end(), buffer, buffer + amt);
}

void png_read(png_structp png, png_bytep buffer, png_size_t amt)
{
    auto ioptr = png_get_io_ptr(png);
    auto rdata = reinterpret_cast<read_data*>(ioptr);
    rdata->read(buffer, amt);
}

void png_flush(png_structp png)
{
    
}

typedef GifFileType *PGifFileType;
void closegif(PGifFileType &pgif)
{
    int error;
    auto rclose = DGifCloseFile(pgif, &error);
    if (rclose == GIF_ERROR) throw std::runtime_error(std::string("error decoding gif: ") + GifErrorString(error));
    pgif = nullptr;
}

void throwgiferror(PGifFileType &pgif)
{
    closegif(pgif);
    throw std::runtime_error(std::string("error decoding gif: ") + GifErrorString(pgif->Error));
}

void blit(const unsigned char* src, int w, int h, unsigned char* dst, int dw, int dh, int top, int left, const GifColorType* colormap, int transparent_color_index)
{
    GifColorType* cdst = reinterpret_cast<GifColorType*>(dst);

    for(int y = 0;y<h;++y)
    {
        for(int x=0;x<w;++x)
        {
            auto spixel = src[y*w + x];
            auto scolor = colormap[spixel];

            if (static_cast<int>(spixel) != transparent_color_index)
            {
                cdst[(y + top)*dw + x + left] = scolor;
            }
        }
    }
}

image image::from_png(const void* sourcedata, size_t amt)
{
    png_structp png = nullptr;
    png_infop info_end = nullptr;
    read_data rdata(0, sourcedata, amt);

    png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, png_error, png_warning);
    if (!png) throw std::runtime_error("could not allocate png write structure");

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_read_struct(&png, &info, &info_end);
        throw std::runtime_error("could not allocate png info structure");
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, &info_end);
        throw std::runtime_error("error with png");
    }

    png_set_read_fn(png, &rdata, png_read);
    png_set_write_fn(png, &rdata, png_write, png_flush);

    png_read_info(png, info);

    auto width = png_get_image_width(png, info);
    auto height = png_get_image_height(png, info);
    auto color_type = png_get_color_type(png, info);
    auto bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png);
    }

    png_color_16 my_background;
    png_color_16p image_background;
    if (png_get_bKGD(png, info, &image_background))
    {
        png_set_background(png, image_background,
                           PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    }
    else
    {
        png_set_background(png, &my_background,
                           PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
    }


    png_read_update_info(png, info);

    image result(width, height, 1);

    auto row_pointers = result.get_row_pointers(0);
    png_read_image(png, &row_pointers[0]);

    return result;
}

image image::from_gif(const void* sourcedata, size_t buffersize)
{
    read_data rdata{ 0, sourcedata, buffersize };
    int error;
    auto pgif = DGifOpen(&rdata, gifreadfunc, &error);
    if (!pgif) throw std::runtime_error(std::string("error decoding gif: ") + GifErrorString(error));

    auto rslurp = DGifSlurp(pgif);
    if (rslurp == GIF_ERROR) throwgiferror(pgif);

    auto frames = pgif->ImageCount;
    auto w = pgif->SWidth;
    auto h = pgif->SHeight;
    image result(w, h, frames);
    auto framesize = w * h * 3;
    GraphicsControlBlock gcb;

    for(auto i=0;i<frames;++i)
    {
        bool first = i == 0;
        const auto& source_image = pgif->SavedImages[i];
        const auto& image_desc = source_image.ImageDesc;
        auto pdestination_frame = result.get_frame_ptr(i);
        auto pprevious_frame = !first ? result.get_frame_ptr(i - 1) : nullptr;
        auto colormap = image_desc.ColorMap
            ? image_desc.ColorMap
            : pgif->SColorMap;

        DGifSavedExtensionToGCB(pgif, i, &gcb);
        result._frame_delay[i] = gcb.DelayTime;

        if (!first && (
            image_desc.Top != 0 ||
            image_desc.Left != 0 ||
            image_desc.Width != w ||
            image_desc.Height != h))
        {
            memcpy_s(pdestination_frame, framesize, pprevious_frame, framesize);
        }

        blit(source_image.RasterBits,
            image_desc.Width,
            image_desc.Height,
            pdestination_frame,
            w, h,
            image_desc.Top,
            image_desc.Left,
            colormap->Colors,
            gcb.TransparentColor);
    }

    closegif(pgif);

    return result;
}

std::vector<unsigned char> image::to_png(int frame)
{
    std::vector<unsigned char> output;
    png_structp png;

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, png_error, png_warning);
    if (!png) throw std::runtime_error("could not allocate png write structure");

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_write_struct(&png, &info);
        throw std::runtime_error("could not allocate png info structure");
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_write_struct(&png, &info);
        throw std::runtime_error("error with png");
    }

    png_set_read_fn(png, &output, png_read);
    png_set_write_fn(png, &output, png_write, png_flush);

    png_set_IHDR(png, info,
        _w, _h,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    auto row_pointers = get_row_pointers(frame);

    png_write_image(png, &row_pointers[0]);
    png_write_end(png, nullptr);

    return output;
}

image::image(int w, int h, int frames) : 
    _w(w),
    _h(h),
    _data(frames * w * h * 3),
    _frame_delay(frames),
    _frames(frames)
{
}

unsigned char* image::get_frame_ptr(int num)
{
    if (num < 0 || num > _frames) throw std::out_of_range("num is out of range");

    return &_data[num * _w * _h * 3];
}

std::vector<unsigned char*> image::get_row_pointers(int frame)
{
    auto pframe = get_frame_ptr(frame);
    size_t pitch = _w * 3;

    std::vector<unsigned char*> pointers(_h);

    for(int i=0;i<_h;++i)
    {
        pointers[i] = pframe + pitch * i;
    }

    return std::move(pointers);
}
