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

    size_t ppos;
    const void* sourcedata;
    const size_t buffersize;
};

struct write_data
{
    write_data() : ppos(0), buffersize(0)
    {
    }

    size_t ppos;
    std::vector<unsigned char> buffer;
    size_t buffersize;
};

int gifreadfunc(GifFileType *handle, GifByteType *destinationbuffer, int amount)
{
    auto rdata = reinterpret_cast<read_data*>(handle->UserData);
    auto& pos = rdata->ppos;
    auto buffersize = rdata->buffersize;
    auto sourcedata = rdata->sourcedata;

    auto src = reinterpret_cast<const unsigned char*>(sourcedata);
    auto dst = reinterpret_cast<unsigned char*>(destinationbuffer);

    auto remain = buffersize - pos;
    auto willread = remain < amount ? remain : amount;

    auto err = memcpy_s(dst, willread, src + pos, willread);
    if (err) throw std::system_error(err, std::system_category());

    pos += willread;
    return static_cast<int>(willread);
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
    png_structp png = nullptr;
    png_infop info = nullptr;

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, png_error, png_warning);
    if (!png) throw std::runtime_error("could not allocate png write structure");

    info = png_create_info_struct(png);
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
    _frames(frames),
    _frame_durations(frames),
    _data(frames * w * h * 3)
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
