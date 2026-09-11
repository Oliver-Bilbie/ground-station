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
  const [lastMessage, setLastMessage] = useState(null);
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
          setLastMessage(data);
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

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(msg);
    }
  };

  return { lastMessage, status, sendMessage };
};
