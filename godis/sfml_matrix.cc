#include "led-matrix.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <SFML/Graphics.hpp>

#define SFML_WINDOW_WIDTH 1280
#define SFML_WINDOW_HEIGHT 720

#define LED_RADIUS_PIXELS 2

namespace rgb_matrix
{

    class SFMLCanvas : public FrameCanvas
    {
    public:
        SFMLCanvas(int height, int width) : FrameCanvas(nullptr), w(width), h(height)
        {
            for (int i = 0; i < w; i++)
            {
                led_matrix.emplace_back();
                for (int j = 0; j < h; j++)
                {
                    sf::CircleShape c(LED_RADIUS_PIXELS, 20);
                    led_matrix[i].push_back(c);
                    int offset = LED_RADIUS_PIXELS + 20;
                    led_matrix[i][j].setPosition(50 + offset + i * (LED_RADIUS_PIXELS + 1) * 2, 240 + offset + j * (LED_RADIUS_PIXELS + 1) * 2 + 1);
                    led_matrix[i][j].setOutlineColor(sf::Color(32, 32, 32));
                    led_matrix[i][j].setOutlineThickness(1);
                }
            }
        }

        virtual ~SFMLCanvas() = default;

        int width() const override { return w; }
        int height() const override { return h; }

        void SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue) override
        {
            if (x < 0 || x >= w || y < 0 || y >= h)
            {
                return;
            }
            led_matrix[x][y].setFillColor({red, green, blue, alpha});
        }

        void SetPixels(int x, int y, int width, int height, Color *colors) override {}
        void Clear() override
        {
            Fill(0, 0, 0);
        }
        void Fill(uint8_t red, uint8_t green, uint8_t blue) override
        {
            for (int i = 0; i < w; i++)
            {
                for (int j = 0; j < h; j++)
                {
                    SetPixel(i, j, red, green, blue);
                }
            }
        }

        const std::vector<std::vector<sf::CircleShape>> &matrix() { return led_matrix; }

        void setAlpha(uint8_t val) { alpha = val; }

    private:
        int w;
        int h;
        uint8_t alpha{255};
        std::vector<std::vector<sf::CircleShape>> led_matrix{};

    };

    class SFMLThread
    {
    public:
        SFMLThread(int w, int h) : th(&SFMLThread::run, this), next_canvas(nullptr)
        {
            tex.loadFromFile("godis.jpg");
            s.setTexture(tex, true);
            s.setPosition({1, -130});
            s.setScale(0.32, 0.32);
            r.setFillColor({60,60,60});
            r.setPosition(45, 257);
            r.setSize({1200, 200});
        }

        ~SFMLThread()
        {
            running = false;
            th.join();
        }

        void run()
        {
            sf::RenderWindow window;
            window.create(sf::VideoMode(SFML_WINDOW_WIDTH, SFML_WINDOW_HEIGHT), "Godis");

            sf::Event event;
            while (window.isOpen() && running)
            {
                // Handle events
                while (window.pollEvent(event))
                {
                    switch (event.type)
                    {
                    case sf::Event::Closed:
                        window.close();
                        break;
                    default:
                        break;
                    }
                }

                if (current_canvas == nullptr)
                    continue;

                window.clear();

                window.draw(s);
                window.draw(r);
                sync.lock();
                for (int i = 0; i < current_canvas->width(); i++)
                {
                    for (int j = 0; j < current_canvas->height(); j++)
                    {
                        window.draw(current_canvas->matrix()[i][j]);
                    }
                }
                sync.unlock();

                if (next_canvas != nullptr)
                {
                    current_canvas = next_canvas;
                    next_canvas = nullptr;
                }

                window.display();
            }
        }

        void setCanvas(SFMLCanvas *c)
        {
            sync.lock();
            current_canvas = c;
            sync.unlock();
        }

        SFMLCanvas *SwapOnVSync(SFMLCanvas *other)
        {
            sync.lock();
            SFMLCanvas *prev = current_canvas;
            next_canvas = other;
            sync.unlock();
            return prev;
        }

    private:
        bool running{true};
        std::thread th;
        std::mutex sync;
        SFMLCanvas *current_canvas;
        SFMLCanvas *next_canvas;

        sf::Texture tex;
        sf::Sprite s;
        sf::RectangleShape r;
    };

    class RGBMatrix::Impl
    {
    public:
        Impl(const Options &options);
        ~Impl();

        FrameCanvas *CreateFrameCanvas();
        FrameCanvas *SwapOnVSync(FrameCanvas *other, unsigned framerate_fraction);

        void SetBrightness(uint8_t val);
        uint8_t brightness();

        void Clear();

        void CanvasCheck()
        {
            if (active_canvas == nullptr)
            {
                CreateFrameCanvas();
            }
        }

    private:
        friend class RGBMatrix; // Necessary evil

        // Brightness
        uint8_t alpha{255};

        Options options;

        SFMLThread *th;
        SFMLCanvas *active_canvas;
        std::vector<SFMLCanvas *> canvases{};

        std::chrono::time_point<std::chrono::steady_clock> last_vsync_swap{};
    };

    RGBMatrix::Impl::Impl(const Options &opt) : options(opt)
    {
        th = new SFMLThread(options.rows, options.cols * options.chain_length);
    }

    void RGBMatrix::Impl::Clear()
    {
        active_canvas->Clear();
    }

    RGBMatrix::Impl::~Impl()
    {
        delete th;
    }

    RGBMatrix::~RGBMatrix()
    {
        delete impl_;
    }

    FrameCanvas *RGBMatrix::Impl::CreateFrameCanvas()
    {
        SFMLCanvas *c = new SFMLCanvas(options.rows, options.cols * options.chain_length);
        c->Fill(255, 255, 255);
        canvases.push_back(c);
        active_canvas = c;
        th->setCanvas(c);

        return c;
    }

    FrameCanvas *RGBMatrix::Impl::SwapOnVSync(FrameCanvas *other, unsigned frame_fraction)
    {
        if (frame_fraction > 1)
        {
            const auto sleep_delay_ms = std::chrono::milliseconds((unsigned)((1.f / (float)frame_fraction) * 1000));
            std::this_thread::sleep_until(last_vsync_swap + sleep_delay_ms);
            last_vsync_swap = std::chrono::steady_clock::now();
        }

        FrameCanvas *prev = th->SwapOnVSync((SFMLCanvas *)other);
        return prev;
    }

    void RGBMatrix::Impl::SetBrightness(uint8_t brightness)
    {
        alpha = (uint8_t)(((float)brightness / 100.f) * 255);
        for (auto *it : canvases)
        {
            it->setAlpha(alpha);
        }
    }

    uint8_t RGBMatrix::Impl::brightness()
    {
        return alpha;
    }

    RGBMatrix *RGBMatrix::CreateFromOptions(const RGBMatrix::Options &options,
                                            const RuntimeOptions &runtime_options)
    {
        //   std::string error;
        //   if (!options.Validate(&error)) {
        //     fprintf(stderr, "%s\n", error.c_str());
        //     return NULL;
        //   }

        //   // For the Pi4, we might need 2, maybe up to 4. Let's open up to 5.
        //   if (runtime_options.gpio_slowdown < 0 || runtime_options.gpio_slowdown > 1000) {
        //     fprintf(stderr, "--led-slowdown-gpio=%d is outside usable range\n",
        //             runtime_options.gpio_slowdown);
        //     return NULL;
        //   }

        //   static GPIO io;  // This static var is a little bit icky.
        //   if (runtime_options.do_gpio_init
        //       && !io.Init(runtime_options.gpio_slowdown)) {
        //     fprintf(stderr, "Must run as root to be able to access /dev/mem\n"
        //             "Prepend 'sudo' to the command\n");
        //     return NULL;
        //   }

        //   if (runtime_options.daemon > 0 && daemon(1, 0) != 0) {
        //     perror("Failed to become daemon");
        //   }

        RGBMatrix::Impl *result = new RGBMatrix::Impl(options);
        // Allowing daemon also means we are allowed to start the thread now.
        // const bool allow_daemon = !(runtime_options.daemon < 0);
        // if (runtime_options.do_gpio_init)
        // result->SetGPIO(&io, allow_daemon);

        // TODO(hzeller): if we disallow daemon, then we might also disallow
        // drop privileges: we can't drop privileges until we have created the
        // realtime thread that usually requires root to be established.
        // Double check and document.
        //   if (runtime_options.drop_privileges > 0) {
        //     drop_privs(runtime_options.drop_priv_user,
        //                runtime_options.drop_priv_group);
        //   }

        return new RGBMatrix(result);
    }

    // Public interface.
    RGBMatrix *RGBMatrix::CreateFromFlags(int *argc, char ***argv,
                                          RGBMatrix::Options *m_opt_in,
                                          RuntimeOptions *rt_opt_in,
                                          bool remove_consumed_options)
    {
        //   RGBMatrix::Options scratch_matrix;
        //   RGBMatrix::Options *mopt = (m_opt_in != NULL) ? m_opt_in : &scratch_matrix;

        //   RuntimeOptions scratch_rt;
        //   RuntimeOptions *ropt = (rt_opt_in != NULL) ? rt_opt_in : &scratch_rt;

        //   if (!ParseOptionsFromFlags(argc, argv, mopt, ropt, remove_consumed_options))
        //     return NULL;
        //   return CreateFromOptions(*mopt, *ropt);

        return nullptr;
    }

    FrameCanvas *RGBMatrix::CreateFrameCanvas()
    {
        return impl_->CreateFrameCanvas();
    }
    FrameCanvas *RGBMatrix::SwapOnVSync(FrameCanvas *other,
                                        unsigned framerate_fraction)
    {
        return impl_->SwapOnVSync(other, framerate_fraction);
    }
    bool RGBMatrix::ApplyPixelMapper(const PixelMapper *mapper)
    {
        // return impl_->ApplyPixelMapper(mapper);
        return false;
    }
    bool RGBMatrix::SetPWMBits(uint8_t value)
    {
        // return impl_->SetPWMBits(value);
        return false;
    }
    uint8_t RGBMatrix::pwmbits()
    {
        // return impl_->pwmbits();
        return 0;
    }

    void RGBMatrix::set_luminance_correct(bool on)
    {
        // return impl_->set_luminance_correct(on);
    }

    bool RGBMatrix::luminance_correct() const
    {
        // return impl_->luminance_correct();
        return false;
    }

    void RGBMatrix::SetBrightness(uint8_t brightness)
    {
        impl_->SetBrightness(brightness);
    }
    uint8_t RGBMatrix::brightness() { return impl_->brightness(); }

    uint64_t RGBMatrix::RequestInputs(uint64_t all_interested_bits)
    {
        // return impl_->RequestInputs(all_interested_bits);
        return 0;
    }
    uint64_t RGBMatrix::AwaitInputChange(int timeout_ms)
    {
        // return impl_->AwaitInputChange(timeout_ms);
        return 0;
    }

    uint64_t RGBMatrix::RequestOutputs(uint64_t all_interested_bits)
    {
        // return impl_->RequestOutputs(all_interested_bits);
        return 0;
    }
    void RGBMatrix::OutputGPIO(uint64_t output_bits)
    {
        // impl_->OutputGPIO(output_bits);
    }

    bool RGBMatrix::StartRefresh()
    {
        // return impl_->StartRefresh();
        return 0;
    }

    // -- Implementation of RGBMatrix Canvas: delegation to ContentBuffer
    int RGBMatrix::width() const
    {
        impl_->CanvasCheck();
        return impl_->active_canvas->width();
    }

    int RGBMatrix::height() const
    {
        impl_->CanvasCheck();
        return impl_->active_canvas->height();
    }

    void RGBMatrix::SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
    {
        impl_->CanvasCheck();
        impl_->active_canvas->SetPixel(x, y, red, green, blue);
    }

    void RGBMatrix::Clear()
    {
        impl_->Clear();
    }

    void RGBMatrix::Fill(uint8_t red, uint8_t green, uint8_t blue)
    {
        impl_->CanvasCheck();
        impl_->active_canvas->Fill(red, green, blue);
    }

    // Makes linker shut up

    // FrameCanvas implementation of Canvas
    FrameCanvas::~FrameCanvas()
    {
        // delete frame_;
    }

    int FrameCanvas::width() const
    {
        return 0;
        // return frame_->width();
    }

    int FrameCanvas::height() const
    {
        return 0;
        // return frame_->height();
    }

    void FrameCanvas::SetPixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
    {
        // frame_->SetPixel(x, y, red, green, blue);
    }

    void FrameCanvas::SetPixels(int x, int y, int width, int height, Color *colors)
    {
        // frame_->SetPixels(x, y, width, height, colors);
    }

    void FrameCanvas::Clear()
    {
        // return frame_->Clear();
    }

    void FrameCanvas::Fill(uint8_t red, uint8_t green, uint8_t blue)
    {
        // frame_->Fill(red, green, blue);
    }

    bool FrameCanvas::SetPWMBits(uint8_t value)
    {
        // return frame_->SetPWMBits(value);
        return false;
    }

    uint8_t FrameCanvas::pwmbits()
    {
        // return frame_->pwmbits();
        return 0;
    }

    // Map brightness of output linearly to input with CIE1931 profile.
    void FrameCanvas::set_luminance_correct(bool on)
    {
        //    frame_->set_luminance_correct(on);
    }

    bool FrameCanvas::luminance_correct() const
    {
        // return frame_->luminance_correct();
        return false;
    }

    void FrameCanvas::SetBrightness(uint8_t brightness)
    {
        // frame_->SetBrightness(brightness);
    }

    uint8_t FrameCanvas::brightness()
    {
        // return frame_->brightness();
        return 0;
    }

    void FrameCanvas::Serialize(const char **data, size_t *len) const
    {
        // frame_->Serialize(data, len);
    }

    bool FrameCanvas::Deserialize(const char *data, size_t len)
    {
        // return frame_->Deserialize(data, len);
        return false;
    }

    void FrameCanvas::CopyFrom(const FrameCanvas &other)
    {
        // frame_->CopyFrom(other.frame_);
    }

    static bool FlagInit(int &argc, char **&argv,
                         RGBMatrix::Options *mopts,
                         RuntimeOptions *ropts,
                         bool remove_consumed_options)
    {
        // argv_iterator it = &argv[0];
        // argv_iterator end = it + argc;

        // std::vector<char *> unused_options;
        // unused_options.push_back(*it++); // Not interested in program name

        // bool bool_scratch;
        // int err = 0;
        // bool posix_end_option_seen = false; // end of options '--'
        // for (/**/; it < end; ++it)
        // {
        // 	posix_end_option_seen |= (strcmp(*it, "--") == 0);
        // 	if (!posix_end_option_seen)
        // 	{
        // 		if (ConsumeStringFlag("gpio-mapping", it, end,
        // 							  &mopts->hardware_mapping, &err))
        // 			continue;
        // 		if (ConsumeStringFlag("rgb-sequence", it, end,
        // 							  &mopts->led_rgb_sequence, &err))
        // 			continue;
        // 		if (ConsumeStringFlag("pixel-mapper", it, end,
        // 							  &mopts->pixel_mapper_config, &err))
        // 			continue;
        // 		if (ConsumeStringFlag("panel-type", it, end,
        // 							  &mopts->panel_type, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("rows", it, end, &mopts->rows, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("cols", it, end, &mopts->cols, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("chain", it, end, &mopts->chain_length, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("parallel", it, end, &mopts->parallel, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("multiplexing", it, end, &mopts->multiplexing, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("brightness", it, end, &mopts->brightness, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("scan-mode", it, end, &mopts->scan_mode, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("pwm-bits", it, end, &mopts->pwm_bits, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("pwm-lsb-nanoseconds", it, end,
        // 						   &mopts->pwm_lsb_nanoseconds, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("pwm-dither-bits", it, end,
        // 						   &mopts->pwm_dither_bits, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("row-addr-type", it, end,
        // 						   &mopts->row_address_type, &err))
        // 			continue;
        // 		if (ConsumeIntFlag("limit-refresh", it, end,
        // 						   &mopts->limit_refresh_rate_hz, &err))
        // 			continue;
        // 		if (ConsumeBoolFlag("show-refresh", it, &mopts->show_refresh_rate))
        // 			continue;
        // 		if (ConsumeBoolFlag("inverse", it, &mopts->inverse_colors))
        // 			continue;
        // 		// We don't have a swap_green_blue option anymore, but we simulate the
        // 		// flag for a while.
        // 		bool swap_green_blue;
        // 		if (ConsumeBoolFlag("swap-green-blue", it, &swap_green_blue))
        // 		{
        // 			if (strlen(mopts->led_rgb_sequence) == 3)
        // 			{
        // 				char *new_sequence = strdup(mopts->led_rgb_sequence);
        // 				new_sequence[0] = mopts->led_rgb_sequence[0];
        // 				new_sequence[1] = mopts->led_rgb_sequence[2];
        // 				new_sequence[2] = mopts->led_rgb_sequence[1];
        // 				mopts->led_rgb_sequence = new_sequence; // leaking. Ignore.
        // 			}
        // 			continue;
        // 		}
        // 		bool allow_hardware_pulsing = !mopts->disable_hardware_pulsing;
        // 		if (ConsumeBoolFlag("hardware-pulse", it, &allow_hardware_pulsing))
        // 		{
        // 			mopts->disable_hardware_pulsing = !allow_hardware_pulsing;
        // 			continue;
        // 		}

        // 		bool request_help = false;
        // 		if (ConsumeBoolFlag("help", it, &request_help) && request_help)
        // 		{
        // 			// In that case, we pretend to have failure in parsing, which will
        // 			// trigger printing the usage(). Typically :)
        // 			return false;
        // 		}

        // 		//-- Runtime options.
        // 		if (ConsumeIntFlag("slowdown-gpio", it, end, &ropts->gpio_slowdown, &err))
        // 			continue;
        // 		if (ropts->daemon >= 0 && ConsumeBoolFlag("daemon", it, &bool_scratch))
        // 		{
        // 			ropts->daemon = bool_scratch ? 1 : 0;
        // 			continue;
        // 		}
        // 		if (ropts->drop_privileges >= 0 &&
        // 			ConsumeBoolFlag("drop-privs", it, &bool_scratch))
        // 		{
        // 			ropts->drop_privileges = bool_scratch ? 1 : 0;
        // 			continue;
        // 		}
        // 		if (ConsumeStringFlag("drop-priv-user", it, end,
        // 							  &ropts->drop_priv_user, &err))
        // 		{
        // 			continue;
        // 		}
        // 		if (ConsumeStringFlag("drop-priv-group", it, end,
        // 							  &ropts->drop_priv_group, &err))
        // 		{
        // 			continue;
        // 		}

        // 		if (strncmp(*it, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0)
        // 		{
        // 			fprintf(stderr, "Option %s starts with %s but it is unknown. Typo?\n",
        // 					*it, OPTION_PREFIX);
        // 		}
        // 	}
        // 	unused_options.push_back(*it);
        // }

        // if (err > 0)
        // {
        // 	return false;
        // }

        // if (remove_consumed_options)
        // {
        // 	// Success. Re-arrange flags to only include the ones not consumed.
        // 	argc = (int)unused_options.size();
        // 	for (int i = 0; i < argc; ++i)
        // 	{
        // 		argv[i] = unused_options[i];
        // 	}
        // }
        return true;
    }

    RuntimeOptions::RuntimeOptions() {}
    RGBMatrix::Options::Options() {}
    bool ParseOptionsFromFlags(int *argc, char ***argv, rgb_matrix::RGBMatrix::Options *m_opt_in, rgb_matrix::RuntimeOptions *rt_opt_in, bool remove_consumed_options)
    {
        if (argc == NULL || argv == NULL)
        {
            fprintf(stderr, "Called ParseOptionsFromFlags() without argc/argv\n");
            return false;
        }
        // Replace NULL arguments with some scratch-space.
        RGBMatrix::Options scratch_matrix;
        RGBMatrix::Options *mopt = (m_opt_in != NULL) ? m_opt_in : &scratch_matrix;

        RuntimeOptions scratch_rt;
        RuntimeOptions *ropt = (rt_opt_in != NULL) ? rt_opt_in : &scratch_rt;

        return FlagInit(*argc, *argv, mopt, ropt, remove_consumed_options);
    }

    void PrintMatrixFlags(FILE *out,
                          const RGBMatrix::Options &defaults,
                          const RuntimeOptions &rt_opt)
    {
    }

}