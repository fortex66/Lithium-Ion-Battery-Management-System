const http = require("http");
const { Server } = require("socket.io");
const app = require("./app");
const tcpServer = require("./services/tcpService");

const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "*", // 모든 도메인에서 접근 가능하도록 설정
    methods: ["GET", "POST"],
  },
});

// Socket.io 설정
io.on("connection", (socket) => {
  console.log("Client connected via Socket.io");

  // `update_relay_state` 이벤트 리스너 추가
  socket.on("update_relay_state", (relayState) => {
    console.log("Received relay state update from client:", relayState);
    require("./controllers/sensorController").handleUpdateRelayState(
      relayState
    );
  });

  socket.on("disconnect", () => {
    console.log("Client disconnected");
  });
});

// Socket.io 인스턴스를 앱 객체에 저장
app.set("io", io);

// 서버 및 tcpServer 시작
server.listen(3000, "0.0.0.0", () => {
  console.log("HTTP server listening on port 3000");
});

tcpServer.listen(9000, "0.0.0.0", () => {
  console.log("TCP server listening on port 9000");
});

module.exports = { server, io };
