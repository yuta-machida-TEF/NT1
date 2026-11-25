#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include"Novice.h"
#include <imgui.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include<cstring>
#include <cstdint>
#include<cstdio>
#include<vector>

#pragma comment(lib, "Ws2_32.lib")

enum class MessageType : uint16_t {
	Text = 1, AssignPlayerId = 2, JoinRequest = 3,
	WorldState = 4, Disconnect = 5,
};
struct PlayerState {

	uint16_t id, x, y;
};
std::vector<PlayerState> players;


// Windowsアプリのmain関数。ImGuiを使うため、Debugで実行する。
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Novice::Initialize("NT1_01_00", 1280, 720);
	char host[64] = "34.104.202.113";
	char port[8] = "8000"; // 01_00・01_01のEcho用ポート
	bool connected = false;

	char receiveBuffer[4096]{};
	int receiveBufferSize = 0;

	//最後に受信した情報
	uint16_t receivedType = 0;
	uint16_t receivedPayloadSize = 0;
	int receivedFrameCount = 0;

	int connectionError = 0;

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

			connectionError = 0;

			addrinfo hints{};
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			addrinfo* address = nullptr;

			//getaddrinfoは、	接続先のIPアドレスやポート番号を取得する
			int result = getaddrinfo(host, port, &hints, &address);

			if (result != 0) {

				connectionError = result;

			} else {

				//socketは、TCP通信を行うためのソケットを作成
				clientSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

				if (clientSocket == INVALID_SOCKET) {

					connectionError = WSAGetLastError();

				} else {

					//connectは、サーバーに接続要求を送るもの
					if (connect(clientSocket, address->ai_addr, static_cast<int>(address->ai_addrlen)) == SOCKET_ERROR) {

						connectionError = WSAGetLastError();

						closesocket(clientSocket);
						clientSocket = INVALID_SOCKET;

					} else {

						connected = true;
					}
				}

				freeaddrinfo(address);
			}

			if (connected) {

				uint16_t payloadSize = 0;
				uint16_t typeValue = static_cast<uint16_t>(MessageType::JoinRequest);

				char sendBuffer[4]{};

				std::memcpy(sendBuffer, &payloadSize, 2);
				std::memcpy(sendBuffer + 2, &typeValue, 2);

				//send()は、サーバーにデータを送信する
				int sentBytes = send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

				if (sentBytes != sizeof(sendBuffer)) {

					connectionError = WSAGetLastError();

					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;
					connected = false;
				}

				if (connected) {

					u_long mode = 1;

					if (ioctlsocket(clientSocket, FIONBIO, &mode) != 0) {

						connectionError = WSAGetLastError();

						closesocket(clientSocket);
						clientSocket = INVALID_SOCKET;
						connected = false;
					}
				}
			}
		}

		
		
            if (connected) {

			    char tempBuffer[1024]{};
			    
			    // サーバーからデータを受信する
			    while (true) {
			    
					//recv()は、サーバーからデータを受信する
			    	 int n = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
			    	 
			    	 if (n > 0) {
			    	 
			    	 	 if (receiveBufferSize + n > sizeof(receiveBuffer)) {
			    	 	 	connected = false;
			    	 	 	break;
			    	 	 }
			    	 	 
			    	 	 std::memcpy(receiveBuffer + receiveBufferSize, tempBuffer, n);
			    	 	 
			    	 	 receiveBufferSize += n;
			    	 
			    	 } else if (n == 0) {
			    	 
			    	 	// サーバーが接続を終了した
			    	 	connected = false;
			    	 	break;
			    	 
			    	 } else {
			    	 
			    	 	if (WSAGetLastError() == WSAEWOULDBLOCK) {
			    	 		break;
			    	 	}
			    	 
			    	 	connected = false;
			    	 	break;
			    	 }
			    }
			    
			    // 受信したメッセージを解析する
			    while (receiveBufferSize >= 4) {
			    
			    	uint16_t payloadSize = 0;
			    	uint16_t typeValue = 0;
			    
			    	// ヘッダーを読み取る
			    	std::memcpy(&payloadSize, receiveBuffer, 2);
			    	std::memcpy(&typeValue, receiveBuffer + 2, 2);
			    
			    	// 本文のサイズを確認
			    	if (payloadSize > 1024) {
			    		connected = false;
			    		break;
			    	}
			    
			    	int frameSize = 4 + payloadSize;
			    
			    	// メッセージ全体が届いていない場合
			    	if (receiveBufferSize < frameSize) {
			    		break;
			    	}
			    
			    	receivedType = typeValue;
			    	receivedPayloadSize = payloadSize;
			    	receivedFrameCount++;
			    
			    	// 本文の先頭
			    	const char* payload = receiveBuffer + 4;
			    
			    	if (receivedType == static_cast<uint16_t>(MessageType::Disconnect)) {
			    
			    		connected = false;
			    		break;
			    
			    	} else if (receivedType == static_cast<uint16_t>(MessageType::WorldState) && payloadSize >= 2) {
			    
			    		uint16_t count = 0;
			    
			    		std::memcpy(&count, payload, 2);
			    
			    		if (count <= 128 && payloadSize == 2 + count * 6) {
			    
			    			players.resize(count);
			    
			    			for (int i = 0; i < count; i++) {
			    
			    				std::memcpy(&players[i].id, payload + 2 + i * 6, 2);
			    
			    				std::memcpy(&players[i].x, payload + 4 + i * 6, 2);
			    
			    				std::memcpy(&players[i].y, payload + 6 + i * 6, 2);
			    			}
			    		}
			    	}
			    
			    	// 処理済みのメッセージを削除する
			    	receiveBufferSize -= frameSize;
			    
			    	std::memmove(receiveBuffer, receiveBuffer + frameSize, receiveBufferSize);
			    }
			    
			    // 通信エラーなどで切断された場合
			    if (!connected) {
					//TCP接続を終了する
			    	closesocket(clientSocket);
			    	clientSocket = INVALID_SOCKET;
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

			players.clear();
			receiveBufferSize = 0;
		}

		 // プレイヤーを描画する
		for (const PlayerState& player : players) {

			uint32_t color = 0x55BD79FF;

			Novice::DrawEllipse(player.x, player.y, 16, 16, 0.0f, color, kFillModeSolid);
		}

		ImGui::EndDisabled();//無効にする範囲を終了する。
		ImGui::Text("Status: %s", connected ? "Connected" : "Not connected");
		ImGui::Text("Received Type: %d", receivedType);
		ImGui::Text("Payload Size: %d", receivedPayloadSize);
		ImGui::Text("Received Frames: %d", receivedFrameCount);
		ImGui::Text("Players: %d", (int)players.size());
		ImGui::Text("Connection Error: %d", connectionError);
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
