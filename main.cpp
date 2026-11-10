#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include"Novice.h"
#include <imgui.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include<cstring>
#include<cstdio>

#pragma comment(lib, "Ws2_32.lib")

// Windowsアプリのmain関数。ImGuiを使うため、Debugで実行する。
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Novice::Initialize("NT1_01_00", 640, 360);
	char host[64] = "34.104.202.113";
	char port[8] = "8001"; // 01_00・01_01のEcho用ポート
	char buffer[256] = "Hello";//サーバーに送信する文字列を保存するもの
	char receiveFrame[1029]{};//サーバーから受信した文字列を保存する
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

			int dataSize = (int)std::strlen(buffer);
			
			sentBytes = 0;
			receivedBytes = 0;

			//前回の受信データを消去
			memset(receiveFrame, 0, sizeof(receiveFrame));


			while(connected && sentBytes < dataSize) {
              
				int n = send(clientSocket,//s: ソケット
					         buffer + sentBytes, //buf: 残りの先頭
					         dataSize - sentBytes,//len : 残りの長さ
					         0);//flags

				if (n == SOCKET_ERROR || n == 0) {
				
					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
					connected = false;

					break;
				}
				sentBytes += n;
			}

			// 受信する目標バイト数
			int expectedSize = dataSize;
			

			// 受信処理
			while (connected && receivedBytes < expectedSize)
			{
				int n = recv(clientSocket,
					         receiveFrame + receivedBytes,
					         expectedSize - receivedBytes,
					         0);
				
				 // 受信エラー
				if (n == SOCKET_ERROR || n == 0) {

					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
					connected = false;

					break;
				}

				// 受信したバイト数を加算
				receivedBytes += n;
	
			}
            
			receiveFrame[receivedBytes] = '\0';
		
		}

		ImGui::EndDisabled();//無効にする範囲を終了する。
		ImGui::Text("Status: %s", connected ? "Connected" : "Not connected");
		ImGui::Text("Sent: %d bytes",sentBytes);
		ImGui::Text("Received: %d bytes", receivedBytes);
		ImGui::Text("Echo: %s", receiveFrame);
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
