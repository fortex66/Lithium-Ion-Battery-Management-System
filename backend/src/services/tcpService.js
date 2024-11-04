const net = require("net");
let raspberryPiSocket = null;
let isFirstDataPacket = true;

const tcpServer = net.createServer((socket) => {
  console.log("Raspberry Pi connected");
  raspberryPiSocket = socket;

  socket.on("data", (data) => {
    try {
      if (isFirstDataPacket) {
        console.log("Ignoring first data packet");
        isFirstDataPacket = false;
        return; // 첫 번째 데이터 패킷을 무시
      }

      let receivedData = data.toString();
      //console.log("Received raw data:", receivedData);
      // 센싱 오류로 nan값이 나오면 parse에서 오류가 발생하므로 null로 변경
      receivedData = receivedData.replace(/\bnan\b/g, null);

      const sensorData = JSON.parse(receivedData);

      //console.log("Parsed sensor data:", sensorData);

      tcpServer.onSensorDataReceived(sensorData);
    } catch (error) {
      console.error("Error parsing data from Raspberry Pi:", error.message);
    }
  });

  socket.on("end", () => {
    console.log("Raspberry Pi disconnected");
    raspberryPiSocket = null;
    isFirstDataPacket = true; // 연결이 끊어지면 플래그를 리셋
  });

  socket.on("error", (err) => {
    console.error("Socket error:", err);
    raspberryPiSocket = null;
    isFirstDataPacket = true; // 오류가 발생해도 플래그를 리셋
  });
});

tcpServer.sendRelayStateToRaspberryPi = (relayState) => {
  if (raspberryPiSocket) {
    raspberryPiSocket.write(relayState);
    console.log("Relay state sent to Raspberry Pi:", relayState);
  } else {
    console.error("Raspberry Pi is not connected");
  }
};

tcpServer.onSensorDataReceived = (sensorData) => {
  console.error("onSensorDataReceived handler is not set");
};

module.exports = tcpServer;
