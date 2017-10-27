#pragma once

class frame;
class image
{
public:
    image(const image& other) = default;
    image(image&& other) = default;

    image& operator=(const image& other) = default;
    image& operator=(image&& other) = default;

    static image from_png(const void* sourcedata, size_t amt);
    static image from_gif(const void* sourcedata, size_t amt);

    std::vector<unsigned char> to_png(int frame);
private:
    friend class frame;

    explicit image(int w, int h, int frames);
    unsigned char* get_frame_ptr(int num);
    std::vector<unsigned char*> get_row_pointers(int frame);

    int _w, _h;
    std::vector<unsigned char> _data;
    std::vector<int> _frame_delay;
    int _frames;
};


class frame
{
private:
    explicit frame(const image &i, int frame)
        :_image(i), _frame(frame)
    {
    }

    friend class image;
    const image _image;
    int _frame;
};