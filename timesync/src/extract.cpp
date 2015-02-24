#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include <rapidjson/document.h>

using rapidjson::Value;

void die(const char *msg) {
    std::fprintf(stderr, "Error: %s\n", msg);
    std::exit(1);
}

struct BgSubtractConfig {
    int history;
    double var_threshold;
    int erosion;
    int dilation;
};

struct Config {
    BgSubtractConfig bg_subtract;
};

void parse_config(BgSubtractConfig &cfg, const Value &value) {
    cfg.history = value["history"].GetInt();
    cfg.var_threshold = value["var_threshold"].GetDouble();
    cfg.erosion = value["erosion"].GetInt();
    cfg.dilation = value["dilation"].GetInt();
}

void parse_config(Config &cfg, const Value &value) {
    parse_config(cfg.bg_subtract, value["bg_subtract"]);
}

std::string read_file(const char *path) {
    std::ifstream fp(path);
    std::string str;
    fp.seekg(0, std::ios::end);
    str.reserve(fp.tellg());
    fp.seekg(0, std::ios::beg);
    str.assign(std::istreambuf_iterator<char>(fp),
               std::istreambuf_iterator<char>());
    return str;
}

Config read_config(const char *path) {
    rapidjson::Document doc;
    {
        std::string contents = read_file(path);
        doc.Parse(contents.c_str());
    }
    Config cfg;
    parse_config(cfg, doc);
    return cfg;
}

struct MatInfo {
    const cv::Mat &m;
};

std::ostream &operator<<(std::ostream &os, const MatInfo &mi) {
    const auto &m = mi.m;
    int type = m.type();
    os << m.cols << 'x' << m.rows << ' ';
    switch (type & CV_MAT_DEPTH_MASK) {
    case CV_8U:  os << "u8";   break;
    case CV_8S:  os << "i8";   break;
    case CV_16U: os << "u16";  break;
    case CV_16S: os << "i16";  break;
    case CV_32S: os << "i32";  break;
    case CV_32F: os << "f32";  break;
    case CV_64F: os << "f64";  break;
    default:     os << "user"; break;
    }
    os << '[' << (1 + (type >> CV_CN_SHIFT)) << ']';
    return os;
}

MatInfo mat_info(const cv::Mat &m) {
    return MatInfo{m};
}

cv::Mat get_kernel(int d) {
    if (d <= 0) {
        return cv::Mat();
    }
    return cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(d*2+1, d*2+1),
        cv::Point(d, d));
}

const unsigned char COLORS[][3] = {
    { 0, 255, 255 },
    { 255, 0, 255 },
    { 255, 255, 0 },
    { 255, 0, 0 },
    { 0, 255, 0 },
    { 0, 0, 255 }
};

cv::Scalar color(int i) {
    int n = sizeof(COLORS) / sizeof(*COLORS);
    int j = i % n;
    if (j < 0) {
        j += n;
    }
    const unsigned char *c = COLORS[j];
    return cv::Scalar(c[0], c[1], c[2]);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        die("Usage: ts_extract SETTINGS.json VIDEO...");
    }

    Config cfg = read_config(argv[1]);
    cv::Mat frame, mask, dilation_kernel, erosion_kernel, mask2;
    cv::VideoCapture cap;
    // These values are OpenCV's defaults, except that we don't want
    // shadows.
    cv::BackgroundSubtractorMOG2 bg_sub(
        cfg.bg_subtract.history,
        cfg.bg_subtract.var_threshold,
        true);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    if (!cap.open(argv[2])) {
        die("Failed to open video.");
    }

    dilation_kernel = get_kernel(cfg.bg_subtract.dilation);
    erosion_kernel = get_kernel(cfg.bg_subtract.erosion);

    cv::namedWindow("Mask", cv::WINDOW_AUTOSIZE);
    for (int frameno = 0; ; frameno++) {
        if (!cap.read(frame) || !frame.data) {
            break;
        }
        if (mask2.empty()) {
            mask2 = mask.clone();
        }
        std::printf("\rframe %d", frameno);
        std::fflush(stdout);
        bg_sub(frame, mask);
        cv::threshold(mask, mask2, 200, 255, cv::THRESH_BINARY);
        std::swap(mask, mask2);
        if (!erosion_kernel.empty()) {
            cv::erode(mask, mask2, erosion_kernel);
            std::swap(mask, mask2);
        }
        if (!dilation_kernel.empty()) {
            cv::dilate(mask, mask2, dilation_kernel);
            std::swap(mask, mask2);
        }
        cv::findContours(
            mask,
            contours,
            hierarchy,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE);
        for (int i = 0, n = contours.size(); i < n; i++) {
            auto c = color(i);
            if (false) {
                cv::drawContours(
                    frame,
                    contours,
                    i,
                    c,
                    1,
                    8,
                    hierarchy);
            }
            auto m = cv::moments(contours[i]);
            cv::Point pt(m.m10 / m.m00, m.m01 / m.m00);
            cv::Size sz(std::sqrt(m.mu20 / m.m00),
                        std::sqrt(m.mu02 / m.m00));
            cv::ellipse(frame, pt, sz, 0, 0, 360, c, 2, 8);
        }
        cv::imshow("Mask", frame);
        int key = cv::waitKey(1);
        if (key >= 0) {
            key &= 0xff;
            if (key == 27) {
                break;
            }
        }
    }
    std::putchar('\n');

    return 0;
}
