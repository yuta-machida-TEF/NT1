#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include"Novice.h"
#include <imgui.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// Windowsアプリのmain関数。ImGuiを使うため、Debugで実行する。
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Novice::Initialize("NT1_01_00", 640, 360);
	char host[64] = "34.104.202.113";
	char port[8] = "8001"; // 01_00・01_01のEcho用ポート
	bool connected = false;

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

		//ソケットとは、プログラムがネットワークを通じてデータを送受信するための窓口です。
		if (ImGui::Button("Connect")) {//ソケットを作成する
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
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!connected);
		if (ImGui::Button("Disconnect")) {//colsesocketを実行し、ソケットを無効化させる
			// TODO 3: ソケットを閉じ、未接続の状態へ戻す。
			//切断するとき
			if (!connected) {
				closesocket(clientSocket);
				clientSocket = INVALID_SOCKET;
			}


			// C++の抜枠： Disconnectボタンの中
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
			connected = false;

			// アプリ終了後(ループ後)
			if (clientSocket != INVALID_SOCKET) {
				closesocket(clientSocket);//TCP通信に使用している、特定のソケットを閉じます。
			}

		}
		ImGui::EndDisabled();
		ImGui::Text("Status: %s", connected ? "Connected" : "Not connected");
		ImGui::End();
#endif
		Novice::EndFrame();
	}

	// TODO 4: 接続が残っていれば閉じ、Winsockを終了する。
	// C++の抜枠： Disconnectボタンの中
	closesocket(clientSocket);
	clientSocket = INVALID_SOCKET;
	connected = false;
	
	//アプリ終了後(ループ後)
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
	}
	WSACleanup();//Winsockの使用を終了するための処理です。ソケット1個だけを閉じる関数ではありません。

	Novice::Finalize();
	return 0;
}
