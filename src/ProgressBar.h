﻿#pragma once
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>

struct ProgressData {
	std::atomic<size_t> processed{ 0 };   // 已处理项
	size_t total{ 0 };                    // 总项数
	std::string name;                   // 进度条名称
	std::chrono::steady_clock::time_point startTime; // 开始时间
	bool finished{ false };               // 是否已完成
};

class ProgressBar {
public:
	static ProgressBar INIFileProgress;
	static ProgressBar CheckerProgress;

	ProgressBar() : stopFlag(false), threadStarted(false) {}

	~ProgressBar() {
		stop(); // 确保析构时停止线程
	}

	void addProgressBar(int id, const std::string& name, size_t total) {
		std::lock_guard<std::mutex> lock(mutex);
		progressBars[id].total = total;
		progressBars[id].name = name;
		progressBars[id].startTime = std::chrono::steady_clock::now();
		start(); // 启动显示线程
	}

	void updateProgress(int id, size_t processed) {
		std::lock_guard<std::mutex> lock(mutex);
		if (progressBars.count(id))
			progressBars[id].processed = processed;
	}

	void markFinished(int id) {
		std::lock_guard<std::mutex> lock(mutex);
		if (progressBars.count(id))
			progressBars[id].finished = true;
	}

	void stop() {
		if (threadStarted) {
			stopFlag = true;
			if (displayThread.joinable())
				displayThread.join();
		}
	}

private:
	std::map<int, ProgressData> progressBars; // 进度条集合
	std::mutex mutex;
	std::thread displayThread;
	std::atomic<bool> stopFlag;
	bool threadStarted;

	void start() {
		if (!threadStarted) {
			threadStarted = true;
			displayThread = std::thread(&ProgressBar::run, this);
		}
	}

	void run() {
		while (!stopFlag) {
			{
				std::lock_guard<std::mutex> lock(mutex);
				int line = 0;
				for (const auto& [id, progress] : progressBars) {
					double percent = progress.total > 0
						? (double)progress.processed / progress.total * 100
						: 0.0;
					auto elapsed = std::chrono::steady_clock::now() - progress.startTime;

					// 输出到控制台
					std::cout << "\033[" << ++line << ";0H"; // 定位光标到行首
					std::cout << "[" << id << "] " << progress.name << " ";

					// 渲染进度条
					size_t barWidth = 50;
					size_t completed = (size_t)(percent / 2);
					std::cout << "\033[32m" // 绿色
						<< std::string(completed, '━')
						<< "\033[90m" // 灰色
						<< std::string(barWidth - completed, '┈')
						<< "\033[0m "; // 重置颜色

					// 显示百分比和时间
					std::cout << std::fixed << std::setprecision(2) << percent << "% ("
						<< formatTimeDuration(elapsed) << ")\n";
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	static std::string formatTimeDuration(std::chrono::steady_clock::duration duration) {
		using namespace std::chrono;
		auto secs = duration_cast<seconds>(duration).count();
		auto mins = secs / 60;
		secs %= 60;
		std::ostringstream oss;
		oss << mins << "m " << secs << "s";
		return oss.str();
	}
};
