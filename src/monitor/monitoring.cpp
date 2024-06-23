#include "monitoring.h"
#include "ui_monitoring.h"
#include "../../include/log.h"

monitoring::monitoring(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::monitoringClass())
	, _bStop(false)
	, _bRecording(false)
{
	ui->setupUi(this);
	this->_init();
	// 设置固定大小
	this->ui->_lableFront->setFixedSize(640, 360);
	this->ui->_lableBack->setFixedSize(640, 360);
	this->setFixedSize(1320, 600);

	// 连接信号与槽
	connect(ui->startRecordingButton, &QPushButton::clicked, this, &monitoring::on_startRecordingButton_clicked);
	connect(ui->stopRecordingButton, &QPushButton::clicked, this, &monitoring::on_stopRecordingButton_clicked);
	connect(ui->playButton, &QPushButton::clicked, this, &monitoring::on_playButton_clicked);
	connect(ui->slider, &QSlider::sliderMoved, this, &monitoring::on_sliderMoved);
}

int monitoring::_init() {
	INIT_LOG();

	// 打开摄像头
	LOG_INFO << "OpenCV Version: " << CV_VERSION;
	_tThread = std::thread([this]() {
		LOG_INFO << "初始化前置摄像头";
		cv::VideoCapture cap_front(1);
		LOG_INFO << "初始化后置摄像头";
		cv::VideoCapture cap_back(0);
		if (!cap_front.isOpened() || !cap_back.isOpened()) {
			std::cerr << "Error: Cannot open camera" << std::endl;
		}

		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		int frameCount = 0;
		double fps = 0.0;
		cv::Mat frame_front, frame_back;
		LOG_WARN << "准备取图";
		while (!this->_bStop) {
			try {
				cap_front >> frame_front;
				cap_back >> frame_back;

				if (frame_front.empty() || frame_back.empty()) {
					LOG_EROR << "Failed to capture frame from camera";
					continue;
				}

				cv::Mat rgb_frame_front;
				cv::cvtColor(frame_front, rgb_frame_front, cv::COLOR_BGR2RGB);
				QImage img_front(rgb_frame_front.data, rgb_frame_front.cols, rgb_frame_front.rows, rgb_frame_front.step, QImage::Format_RGB888);

				cv::Mat rgb_frame_back;
				cv::cvtColor(frame_back, rgb_frame_back, cv::COLOR_BGR2RGB);
				QImage img_back(rgb_frame_back.data, rgb_frame_back.cols, rgb_frame_back.rows, rgb_frame_back.step, QImage::Format_RGB888);

				QMetaObject::invokeMethod(ui->_lableFront, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, QPixmap::fromImage(img_front)));
				QMetaObject::invokeMethod(ui->_lableBack, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, QPixmap::fromImage(img_back)));

				if (_bRecording) {
					_videoWriter.write(frame_front);
				}

				frameCount++;
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				std::chrono::duration<double> elapsedTime = now - start;

				if (elapsedTime.count() > 1.0) {
					fps = frameCount / elapsedTime.count();
					LOG_INFO << "实时帧率(FPS): " << fps;
					ui->_lableLog->setText(QString::fromLocal8Bit("实时帧率(FPS): %1").arg(fps));
					frameCount = 0;
					start = now;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			catch (const std::exception&) {
				LOG_EROR << "Failed to capture frame from camera";
			}
			catch (...) {
				LOG_EROR << "捕获到未知异常";
			}
		}
		});

	LOG_INFO << "初始化成功";
	return 0;
}

monitoring::~monitoring() {
	this->_bStop = true;
	if (_tThread.joinable()) {
		_tThread.join();
	}
	delete ui;
}

void monitoring::on_startRecordingButton_clicked() {
	_videoFileName = "recorded_video.avi";
	int frameWidth = 640;
	int frameHeight = 360;
	_videoWriter.open(_videoFileName, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 10, cv::Size(frameWidth, frameHeight), true);
	if (!_videoWriter.isOpened()) {
		LOG_EROR << "Cannot open video file for writing";
		return;
	}
	_bRecording = true;
	LOG_INFO << "Started recording";
}

void monitoring::on_stopRecordingButton_clicked() {
	_bRecording = false;
	_videoWriter.release();
	LOG_INFO << "Stopped recording";
}

void monitoring::on_playButton_clicked() {
	std::thread([this]() {
		cv::VideoCapture cap(_videoFileName);
		if (!cap.isOpened()) {
			LOG_EROR << "Cannot open video file for playback";
			return;
		}

		int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
		ui->slider->setMaximum(totalFrames);

		cv::Mat frame;
		while (cap.read(frame) && !_bStop) {
			cv::Mat rgb_frame;
			cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
			QImage img(rgb_frame.data, rgb_frame.cols, rgb_frame.rows, rgb_frame.step, QImage::Format_RGB888);

			QMetaObject::invokeMethod(ui->_lableFront, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, QPixmap::fromImage(img)));

			int currentFrame = static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES));
			QMetaObject::invokeMethod(ui->slider, "setValue", Qt::QueuedConnection, Q_ARG(int, currentFrame));

			std::this_thread::sleep_for(std::chrono::milliseconds(30));  // Adjust as needed
		}
		}).detach();
}

void monitoring::on_sliderMoved(int position) {
	std::thread([this, position]() {
		cv::VideoCapture cap(_videoFileName);
		if (!cap.isOpened()) {
			LOG_EROR << "Cannot open video file for seeking";
			return;
		}

		cap.set(cv::CAP_PROP_POS_FRAMES, position);

		cv::Mat frame;
		if (cap.read(frame)) {
			cv::Mat rgb_frame;
			cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
			QImage img(rgb_frame.data, rgb_frame.cols, rgb_frame.rows, rgb_frame.step, QImage::Format_RGB888);

			QMetaObject::invokeMethod(ui->_lableFront, "setPixmap", Qt::QueuedConnection, Q_ARG(QPixmap, QPixmap::fromImage(img)));
		}
		}).detach();
}
