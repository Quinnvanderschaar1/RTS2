#include <vector>
#include <fstream>
#include <string>

struct TimingSample {
    uint64_t block;
    uint64_t latencyNs;
};

class TimingLogger {
public:
    void add(uint64_t block, uint64_t latencyNs) {
        samples.push_back({block, latencyNs});
    }

    void saveCSV(const std::string& filename) {
        std::ofstream file(filename);

        file << "block,latency_ns,latency_ms\n";

        for (const auto& s : samples) {
            file << s.block << ","
                 << s.latencyNs << ","
                 << (s.latencyNs / 1e6)
                 << "\n";
        }

        file.close();
    }

private:
    std::vector<TimingSample> samples;
};