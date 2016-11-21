namespace HDRPlusPipeline
{
    class Image
    {

    private:
        double *p;
        size_t w, h;

        void toneMap();

    public:       

        Image()
        {
            w = h = 0;
            p = NULL;
        }

        Image(size_t width, size_t height, double *pixels) w(width), h(height), p(pixels) {}

        Image(std::string filename) {
            read(filename);
        }

        inline size_t width() {
            return w;
        }

        inline size_t height() {
            return h;
        }

        inline double* pixels() {
            return p;
        }

        inline double pixel(size_t x, size_t y, size_t c) {
            if (x < 0 || x >= w || y < 0 || y >= h || c < 0 || c >= 3) {
                throw std::out_of_range("Tried accessing a pixel out of the image boundaries");
            }
            return p[3 * (y * w + x) + c];
        }

        void read(std::string filename);
        void write(std::string filename);
        void finish();
    };
}

