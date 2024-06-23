#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>

namespace Ui {
	class monitoringClass;
}

class monitoring : public QMainWindow
{
	Q_OBJECT

public:
	explicit monitoring(QWidget* parent = nullptr);
	~monitoring();

private slots:
	void on_startRecordingButton_clicked();
	void on_stopRecordingButton_clicked();
	void on_playButton_clicked();
	void on_sliderMoved(int position);

private:
	int _init();
	void _recordVideo();
	void _playVideo();

	Ui::monitoringClass* ui;
	std::thread _tThread;
	std::atomic<bool> _bStop;
	std::atomic<bool> _bRecording;
	cv::VideoWriter _videoWriter;
	std::string _videoFileName;
};
