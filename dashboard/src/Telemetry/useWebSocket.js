import { useEffect, useRef, useState } from "react";

export const useWebSocket = (url) => {
  const [lastMessage, setLastMessage] = useState(null);
  const [status, setStatus] = useState("Connecting");
  const socketRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const isMountedRef = useRef(true);
  const retryCountRef = useRef(0);

  useEffect(() => {
    isMountedRef.current = true;

    const connect = () => {
      setStatus("Connecting");

      const socket = new WebSocket(url);
      socketRef.current = socket;

      socket.onopen = () => {
        if (!isMountedRef.current) return;
        setStatus("Open");
        retryCountRef.current = 0;
      };

      socket.onmessage = (event) => {
        if (!isMountedRef.current) return;
        setLastMessage(event.data);
      };

      socket.onclose = () => {
        if (!isMountedRef.current) return;
        setStatus("Closed");
        // Exponential backoff: min 2s, max 5 min
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

    return () => {
      isMountedRef.current = false;
      socketRef.current?.close();
      clearTimeout(reconnectTimeoutRef.current);
    };
  }, [url]);

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(msg);
    }
  };

  return { lastMessage, status, sendMessage };
};
