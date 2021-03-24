#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <queue>
#include <numeric>
#include <algorithm>

using namespace std;

float noise_th= 0.25;    //noise threshold
const float N = 4.7;
uint32_t extract_SF(const char * wav_path)
{
	uint32_t SF;
	vector<char> header(44);
	ifstream wav_file;
	wav_file.open(wav_path, ios::binary|ios::ate);
	wav_file.seekg(0, ios::beg);
	wav_file.read(&header[0], 44);
	wav_file.close();
	memcpy(&SF, &header[24], 4);
	return SF;
}

vector<float> extract_sample(const char * wav_path)
{
	ifstream wav_file;
	wav_file.open(wav_path, ios::binary|ios::ate);
	size_t wav_size = wav_file.tellg();
	size_t sample_count = (wav_size - 44)/2;	//16bit samples
	vector<char> buffer(wav_size);
	vector<int16_t> wav_sample(sample_count);
	vector<float> sample_vect(sample_count);
	wav_file.seekg(0, ios::beg);
	wav_file.read(&buffer[0], wav_size);
	wav_file.close();
	memcpy(&wav_sample[0], &buffer[0] + 44, wav_size - 44);
	for(size_t i = 0; i < sample_count; i++)
	{
		sample_vect[i] = float(wav_sample[i])/float(32767);
	}
	return sample_vect;
}


float calculate_SNR(queue<float> &signal_fifo)
{
    vector<float> noise_vect;
    vector<float> speech_vect;
    float SNR;
    float noise_std;
    float speech_std;
    auto fast_std = [](vector<float> input) //https://stackoverflow.com/questions/7616511/calculate-mean-and-standard-deviation-from-a-vector-of-samples-in-c-using-boos/12405793#12405793
    {
        float mean, acc;
        mean = accumulate(begin(input), end(input), 0.0)/input.size();
        acc = 0.0;
        for_each(begin(input), end(input), [&](const float d)
        {
            acc += (d - mean)*(d - mean);
        });
        return sqrt(acc/(input.size()-1));
    };

    while(!signal_fifo.empty()) //Seprate speech and noise samples(fifo has the ABS of input signal)
    {
        if(signal_fifo.front() < noise_th)
        {
            noise_vect.push_back(signal_fifo.front());
            signal_fifo.pop();
        }
        else
        {
            speech_vect.push_back(signal_fifo.front());
            signal_fifo.pop();
        }
    }

    noise_std = fast_std(noise_vect);
    speech_std = fast_std(speech_vect);
    SNR = 20*log10(speech_std/noise_std);
    noise_th = N*noise_std;     //Update threshold value
    return SNR;
}

int main(int argc, const char * argv[])
{
	float ave = 0;
	queue<float> signal_fifo;
	size_t w_len{pow(2,17)};		//window size in samples
	const char *wav_path = "test.wav";
	uint32_t SF = extract_SF(wav_path);
	vector<float> sample_vect = extract_sample(wav_path);

		for(size_t i = 0; i < sample_vect.size(); i ++)
		{
			float * tmp = (&sample_vect[i]);
			signal_fifo.push(abs(sample_vect[i]));	//add to FIFO for context-awareness
			if(signal_fifo.size() == w_len) 
			{
				ave += calculate_SNR(signal_fifo);	//estimate SNR every w_len samples
			}
		}
		cout<<"\nN =\t"<<"\tAVE:\t"<<ave/(sample_vect.size()/pow(2,17)) <<"\n";
	return 0;
}