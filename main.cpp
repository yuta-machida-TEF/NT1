#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include"Novice.h"
#include <imgui.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include<cstring>
#include <cstdint>
#include<cstdio>

#pragma comment(lib, "Ws2_32.lib")

// ========================================
// 全データを送信する関数
// ========================================

bool SendAll(SOCKET socket, const char* data, int size, int& sentBytes) {

	sentBytes = 0;

	while (sentBytes < size) {

		int n = send(socket, data + sentBytes, size - sentBytes, 0);

		if (n <= 0) {
			return false;
		}

		sentBytes += n;
	}

	return true;
}

// ========================================
// 指定したサイズを受信する関数
// ========================================

bool RecvAll(SOCKET socket, char* data, int size, int& receivedBytes) {

	receivedBytes = 0;

	while (receivedBytes < size) {

		int n = recv(socket, data + receivedBytes, size - receivedBytes, 0);

		if (n <= 0) {
			return false;
		}

		receivedBytes += n;
	}

	return true;
}

// Windowsアプリのmain関数。ImGuiを使うため、Debugで実行する。
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Novice::Initialize("NT1_01_00", 640, 360);
	char host[64] = "34.104.202.113";
	char port[8] = "8000"; // 01_00・01_01のEcho用ポート
	char buffer[256] = "Hello";//サーバーに送信する文字列を保存するもの
	char sendFrame[260]{};//サーバーから受信した文字列を保存する
	char receiveFrame[1025 + 1]{};
	char header[1025]{};
	bool connected = false;

	int sentBytes = 0;
	int receivedBytes = 0;

	// TODO 1: 通信用の変数を用意し、Winsockを初期化する。
	//通信を使い始めるとき
	WSADATA wsaData{};
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET clientSocket = INVALID_SOCKET;

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();
#ifdef USE_IMGUI
		ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Once);
		ImGui::Begin("TCP Connection");
		ImGui::BeginDisabled(connected);
		ImGui::InputText("Host", host, sizeof(host));
		ImGui::InputText("Port", port, sizeof(port));
		ImGui::EndDisabled();


		//ソケットとは、プログラムがネットワークを通じてデータを送受信するための窓口です。
		// ソケットを作成する
		if (!connected && ImGui::Button("Connect")) {
			// TODO 2: 接続先を取得し、ソケットを作って接続する。
			addrinfo hints{};
			hints.ai_family = AF_INET;//IPv4
			hints.ai_socktype = SOCK_STREAM;//TCP向け
			addrinfo* address = nullptr;
			if (getaddrinfo(host,port,&hints,&address) == 0) {
				clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
			    connected = connect(clientSocket, address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0;
				freeaddrinfo(address);
			}

		}
		
		ImGui::SameLine();
		ImGui::BeginDisabled(!connected);
		if (ImGui::Button("Disconnect")) {
			// TODO 3: ソケットを閉じ、未接続の状態へ戻す。
			// C++の抜枠： Disconnectボタンの中
			closesocket(clientSocket);//colsesocketを実行し、ソケットを無効化させる
			clientSocket = INVALID_SOCKET;
			connected = false;
		}
		ImGui::InputText("Text", buffer, sizeof(buffer));
		if (ImGui::Button("Send") && buffer[0] != '\0') {

			sentBytes = 0;
			receivedBytes = 0;

			memset(sendFrame, 0, sizeof(sendFrame));
			memset(receiveFrame, 0, sizeof(receiveFrame));
			memset(header, 0, sizeof(header));

		
              // ====================================
			// ① 本文のサイズを取得
			// ====================================

			int dataSize = static_cast<int>(strlen(buffer));

			// ====================================
			// ② ヘッダーを作成
			// ====================================

			uint16_t bodySize = htons(static_cast<uint16_t>(dataSize));

			uint16_t type = htons(1);

			memcpy(header, &bodySize, 2);
			memcpy(header + 2, &type, 2);

			// ====================================
			// ③ ヘッダー＋本文を連結
			// ====================================

			memcpy(sendFrame, header,4);

			memcpy(sendFrame + 4, buffer, dataSize);

			int totalSize = 4 + dataSize;

			// ====================================
			// ④ 全体を送信
			// ====================================

			bool sendSuccess = SendAll(clientSocket, sendFrame, totalSize, sentBytes);

			if (!sendSuccess) {

				closesocket(clientSocket);
				clientSocket = INVALID_SOCKET;
				connected = false;
			}

			// ====================================
			// ⑤ ヘッダーを4バイト受信
			// ====================================

			if (connected) {

				int headerBytes = 0;

				bool headerSuccess = RecvAll(clientSocket, header, 4, headerBytes);

				receivedBytes = headerBytes;

				if (!headerSuccess) {

					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
					connected = false;
				}
			}

			// ====================================
			// ⑥ ヘッダーを解析
			// ====================================

			if (connected) {

				uint16_t receivedBodySize = 0;
				uint16_t receivedType = 0;

				memcpy(&receivedBodySize, header, 2);

				memcpy(&receivedType, header + 2, 2);

				// ネットワークバイトオーダーから変換
				int expectedSize = ntohs(receivedBodySize);

				int typeValue = ntohs(receivedType);

				// 本文長と種別を確認
				if (expectedSize > 1024 || expectedSize < 0 || typeValue != 1) {

					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
					connected = false;
				}

				// ====================================
				// ⑦ 本文を受信
				// ====================================

				if (connected) {

					int bodyBytes = 0;

					bool bodySuccess = RecvAll(clientSocket, receiveFrame, expectedSize, bodyBytes);

					receivedBytes += bodyBytes;

					if (!bodySuccess) {

						closesocket(clientSocket);
						clientSocket = INVALID_SOCKET;
						connected = false;
					} else {

						// ====================================
						// ⑧ 終端文字を追加
						// ====================================

						receiveFrame[expectedSize] = '\0';
					}
				}
			}
		}

			

		ImGui::EndDisabled();//無効にする範囲を終了する。
		ImGui::Text("Status: %s", connected ? "Connected" : "Not connected");
		ImGui::Text("Sent: %d bytes", sentBytes);
		ImGui::Text("Received: %d bytes", receivedBytes);
		ImGui::Text("Reply: %s", receiveFrame);
		ImGui::End();
#endif
		Novice::EndFrame();
	}

	// TODO 4: 接続が残っていれば閉じ、Winsockを終了する。
	//アプリ終了後(ループ後)
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
	}
	WSACleanup();//Winsockの使用を終了するための処理です。ソケット1個だけを閉じる関数ではありません。

	Novice::Finalize();
	return 0;
}
