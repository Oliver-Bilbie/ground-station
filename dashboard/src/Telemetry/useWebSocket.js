import { useEffect, useRef, useState } from "react";
import { ATTACH_ENDPOINT } from "../helpers/endpoints";
import { parsePacket } from "../helpers";

const LOCAL_WS = "ws://localhost:9001";
const LEASE_REFRESH_MS = 8 * 60 * 1000;
const isLocal = ATTACH_ENDPOINT.includes("PLACEHOLDER");

const attach = async () => {
  const res = await fetch(ATTACH_ENDPOINT, { method: "POST" });
  const data = await res.json();
  if (!res.ok || data.error) {
    throw new Error(data.error || `attach failed: ${res.status}`);
  }
  return data;
};

export const useWebSocket = () => {
  const satelliteData = useRef({});
  const hasNewData = useRef(false);
  const satelliteEvents = useRef([]);
  const [status, setStatus] = useState("Connecting");
  const socketRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const leaseIntervalRef = useRef(null);
  const isMountedRef = useRef(true);
  const retryCountRef = useRef(0);
  const generationRef = useRef(0);

  useEffect(() => {
    isMountedRef.current = true;

    const connect = async () => {
      const generation = ++generationRef.current;
      setStatus("Connecting");

      let url = LOCAL_WS;
      let protocols;
      if (!isLocal) {
        try {
          const creds = await attach();
          if (!isMountedRef.current || generation !== generationRef.current) {
            return;
          }
          url = creds.wsUrl;
          protocols = creds.protocols;
        } catch (err) {
          console.error("Attach error:", err);
          if (!isMountedRef.current || generation !== generationRef.current) {
            return;
          }
          setStatus("Closed");
          const retryDelay = Math.min(
            300000,
            2000 * 2 ** retryCountRef.current,
          );
          retryCountRef.current += 1;
          reconnectTimeoutRef.current = setTimeout(connect, retryDelay);
          return;
        }
      }

      const socket = protocols
        ? new WebSocket(url, protocols)
        : new WebSocket(url);
      socketRef.current = socket;

      socket.onopen = () => {
        if (!isMountedRef.current || generation !== generationRef.current) {
          return;
        }
        setStatus("Open");
        retryCountRef.current = 0;
      };

      socket.onmessage = (event) => {
        if (!isMountedRef.current || generation !== generationRef.current) {
          return;
        }
        try {
          const data = parsePacket(event.data);
          switch (data.event) {
            case "position":
              const posSat = (satelliteData.current[data.satellite_id] ??= {});
              posSat.position = data.position;
              hasNewData.current = true;
              break;

            case "latency":
              const latSat = (satelliteData.current[data.satellite_id] ??= {});
              latSat.latency = data.latency;
              break;

            case "dropped_packet":
            case "unavailable_packet":
            case "lost_packet_report":
              satelliteEvents.current.push(data);
              break;

            case "disconnect":
              delete satelliteData.current[data.satellite_id];
              satelliteEvents.current.push(data);
              hasNewData.current = true;
              break;

            default:
              console.warn(`Received unexpected packet type: ${data.event}`);
          }
        } catch {}
      };

      socket.onclose = () => {
        if (!isMountedRef.current || generation !== generationRef.current) {
          return;
        }
        setStatus("Closed");
        const retryDelay = Math.min(300000, 2000 * 2 ** retryCountRef.current);
        retryCountRef.current += 1;
        reconnectTimeoutRef.current = setTimeout(connect, retryDelay);
      };

      socket.onerror = (err) => {
        console.error("WebSocket error:", err);
        socket.close();
      };
    };

    connect();

    if (!isLocal) {
      leaseIntervalRef.current = setInterval(() => {
        attach().catch((err) => console.error("Lease refresh failed:", err));
      }, LEASE_REFRESH_MS);
    }

    return () => {
      isMountedRef.current = false;
      generationRef.current += 1;
      socketRef.current?.close();
      clearTimeout(reconnectTimeoutRef.current);
      clearInterval(leaseIntervalRef.current);
    };
  }, []);

  const readData = (claim) => {
    const isNew = hasNewData.current;
    if (claim) hasNewData.current = false;
    return { dataRef: satelliteData, isNew };
  };

  const readEvents = () => {
    return satelliteEvents.current.splice(0);
  };

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(msg);
    }
  };

  return {
    satelliteData: readData,
    satelliteEvents: readEvents,
    status,
    sendMessage,
  };
};
