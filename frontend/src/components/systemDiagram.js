import React, { useState, useEffect } from "react";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faChargingStation } from "@fortawesome/free-solid-svg-icons";
import styled from "styled-components";
import { io } from "socket.io-client";

// 소켓 설정
const socket = io("http://192.168.0.152:3000");

function BatterySystemDiagram({ data }) {
  const getChargeModeText = (mode, batteryPercentage, relayState) => {
    if (relayState === 1) {
      return "방전";
    }
    if (batteryPercentage === 100) {
      return "충전 완료";
    }
    switch (mode) {
      case 0:
        return "충전 없음";
      case 1:
        return "저속 충전";
      case 2:
        return "고속 충전";
      default:
        return "알 수 없음";
    }
  };

  const [batteryStates, setBatteryStates] = useState(
    new Array((data.temperature || []).length).fill("충전")
  );

  useEffect(() => {
    if (data.relayState) {
      setBatteryStates(
        data.relayState.map((state) => (state === 0 ? "충전" : "방전"))
      );
    }
  }, [data.relayState]);

  const handleIconClick = (idx) => {
    if (idx >= data.relayState.length) return; // 배열 인덱스 안전성 검사

    setBatteryStates((prevStates) => {
      const newStates = [...prevStates];
      newStates[idx] = newStates[idx] === "충전" ? "방전" : "충전";
      return newStates;
    });

    const newRelayState = [...data.relayState];
    newRelayState[idx] = newRelayState[idx] === 0 ? 1 : 0;
    data.relayState = newRelayState;
    const relayStateString = newRelayState.join("");
    socket.emit("update_relay_state", relayStateString); // 문자열로 변환하여 전송
  };

  const getIconColor = (chargeMode, relayState, batteryState) => {
    if (relayState === 1) {
      return "red";
    }
    if (chargeMode === 0) {
      return "black";
    }
    return batteryState === "충전" ? "green" : "black";
  };

  return (
    <Container>
      <Title>Battery Control System</Title>
      <DiagramContainer>
        <svg width="1200" height="550">
          <defs>
            <linearGradient
              id="greenGradient"
              x1="0%"
              y1="0%"
              x2="100%"
              y2="100%"
            >
              <stop
                offset="0%"
                style={{ stopColor: "lightgreen", stopOpacity: 1 }}
              />
              <stop
                offset="100%"
                style={{ stopColor: "green", stopOpacity: 1 }}
              />
            </linearGradient>
            <linearGradient
              id="redGradient"
              x1="0%"
              y1="0%"
              x2="100%"
              y2="100%"
            >
              <stop
                offset="0%"
                style={{ stopColor: "lightcoral", stopOpacity: 1 }}
              />
              <stop
                offset="100%"
                style={{ stopColor: "red", stopOpacity: 1 }}
              />
            </linearGradient>
            <linearGradient
              id="circleGradient"
              x1="0%"
              y1="0%"
              x2="100%"
              y2="100%"
            >
              <stop offset="0%" style={{ stopColor: "#f0f", stopOpacity: 1 }} />
              <stop
                offset="100%"
                style={{ stopColor: "#0ff", stopOpacity: 1 }}
              />
            </linearGradient>
            <linearGradient
              id="chargingGradient"
              x1="0%"
              y1="0%"
              x2="100%"
              y2="100%"
            >
              <stop offset="0%" stopColor="lightgreen">
                <animate
                  attributeName="stop-color"
                  values="lightgreen;green;lightgreen"
                  dur="3s"
                  repeatCount="indefinite"
                />
              </stop>
              <stop offset="100%" stopColor="green">
                <animate
                  attributeName="stop-color"
                  values="green;lightgreen;green"
                  dur="3s"
                  repeatCount="indefinite"
                />
              </stop>
            </linearGradient>
            <filter id="glow">
              <feGaussianBlur stdDeviation="4.5" result="coloredBlur" />
              <feMerge>
                <feMergeNode in="coloredBlur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>
            <style>
              {`
                @keyframes blink {
                  0% { opacity: 1; }
                  50% { opacity: 0; }
                  100% { opacity: 1; }
                }
              `}
            </style>
          </defs>

          {(data.temperature || []).map((temp, idx) => (
            <g key={idx} transform={`translate(${idx * 385 - 5}, 0)`}>
              <circle
                cx="155"
                cy="60"
                r="40"
                stroke="url(#circleGradient)"
                strokeWidth="4"
                fill="none"
                filter="url(#glow)"
              />
              {(data.fanSpeed[idx] > 0 || data.relayState[idx] === 1) && (
                <g transform="translate(155, 60)">
                  {[...Array(4)].map((_, i) => (
                    <polygon
                      key={i}
                      points="-3,-30 3,-30 0,0"
                      fill="url(#circleGradient)"
                    >
                      <animateTransform
                        attributeName="transform"
                        type="rotate"
                        from={`${i * 90} 0 0`}
                        to={`${(i + 1) * 90} 0 0`}
                        dur={`${
                          data.relayState[idx] === 1
                            ? 1 / (data.resisterFanPwm / 7)
                            : data.chargeMode[idx] === 0
                            ? "0s"
                            : 1 / (data.fanSpeed[idx] / 7)
                        }s`}
                        repeatCount="indefinite"
                      />
                    </polygon>
                  ))}
                </g>
              )}
              <text
                x="260"
                y="60"
                fontWeight={700}
                fill={
                  data.relayState[idx] === 1
                    ? "red"
                    : data.chargeMode[idx] !== 0
                    ? "green"
                    : "black"
                }
                textAnchor="left"
              >
                {data.relayState && data.relayState[idx] === 0
                  ? "충전 팬"
                  : "방전 팬"}{" "}
                :{" "}
                {data.relayState[idx] === 1
                  ? data.resisterFanPwm
                  : data.chargeMode[idx] === 0
                  ? 0
                  : data.fanSpeed[idx]}
                %
              </text>

              <rect
                x="104"
                y="159"
                width="100"
                height="202"
                fill="white"
                stroke="black"
                strokeWidth="3"
                rx="15"
                ry="15"
                style={{ filter: "drop-shadow(0 0 5px rgba(0,0,0,0.5))" }}
              />
              <rect x="144" y="140" width="20" height="20" fill="black" />
              <rect
                x="105"
                y={
                  360 -
                  200 *
                    ((data.batteryPercentage && data.batteryPercentage[idx]) /
                      100)
                }
                width="98"
                height={
                  200 *
                  ((data.batteryPercentage && data.batteryPercentage[idx]) /
                    100)
                }
                fill={
                  temp !== null && temp >= 15 && temp <= 45
                    ? data.batteryPercentage[idx] < 15
                      ? "yellow"
                      : data.chargeMode[idx] !== 0 && data.relayState[idx] !== 1
                      ? "url(#chargingGradient)"
                      : "limegreen"
                    : "red"
                }
                rx="15"
                ry="15"
              />

              <text
                x="155"
                y="330"
                fill="black"
                fontWeight={700}
                textAnchor="middle"
              >
                {(data.batteryPercentage && data.batteryPercentage[idx]) ?? 0}%
              </text>
              <rect
                x="245"
                y="180"
                width="150"
                height="150"
                stroke="black"
                strokeWidth="1.5"
                fill="none"
                rx="10"
                ry="10"
              />
              <text
                x="260"
                y="210"
                fill="black"
                fontWeight={700}
                textAnchor="left"
              >
                {data.relayState && data.relayState[idx] === 0
                  ? "충전 온도"
                  : "방전 온도"}{" "}
                :{" "}
                {data.relayState && data.relayState[idx] === 0
                  ? temp?.toFixed(1) ?? 0
                  : data.resisterTemp[idx]?.toFixed(1) ?? 0}
                °C
              </text>
              <text
                x="260"
                y="260"
                fill="black"
                fontWeight={700}
                textAnchor="left"
              >
                {data.relayState && data.relayState[idx] === 0
                  ? "충전 전압"
                  : "방전 전압"}{" "}
                :{" "}
                {data.relayState[idx] === 0 &&
                data.chargeMode &&
                data.chargeMode[idx] === 0
                  ? 0
                  : (data.voltage && data.voltage[idx]?.toFixed(2)) ?? 0}
                V
              </text>
              <text
                x="260"
                y="310"
                fill="black"
                fontWeight={700}
                textAnchor="left"
              >
                {data.relayState && data.relayState[idx] === 0
                  ? "충전 전류"
                  : "방전 전류"}{" "}
                :{" "}
                {data.relayState[idx] === 0 &&
                data.chargeMode &&
                data.chargeMode[idx] === 0
                  ? 0
                  : (data.current && (data.current[idx] / 1000).toFixed(1)) ??
                    0}
                A
              </text>
              <foreignObject
                x="125"
                y="440"
                width="120"
                height="120"
                onClick={() => handleIconClick(idx)}
              >
                <FontAwesomeIcon
                  icon={faChargingStation}
                  size="4x"
                  color={getIconColor(
                    data.chargeMode && data.chargeMode[idx],
                    data.relayState[idx],
                    batteryStates[idx]
                  )}
                  style={{ cursor: "pointer" }}
                />
              </foreignObject>
              <text
                x="320"
                y="480"
                fill="black"
                textAnchor="middle"
                fontSize="24"
                fontWeight={900}
                style={
                  data.batteryPercentage[idx] === 100
                    ? { animation: "blink 1s infinite" }
                    : {}
                }
              >
                {getChargeModeText(
                  data.chargeMode && data.chargeMode[idx],
                  data.batteryPercentage && data.batteryPercentage[idx],
                  data.relayState[idx]
                )}
              </text>
            </g>
          ))}
        </svg>
      </DiagramContainer>
    </Container>
  );
}

export default BatterySystemDiagram;

const Container = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  background: #f0f4f8;
  box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.2);
`;

const Title = styled.div`
  font-size: 30px;
  font-weight: 600;
  margin-top: 20px;
  margin-bottom: 20px;
  color: #333;
  text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
`;

const DiagramContainer = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  width: 100%;
  padding: 20px 0px;
  overflow-x: auto;
  background: #ecebff;
  border-radius: 30px;
  box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.4);
  position: relative;
  z-index: 0;
`;
