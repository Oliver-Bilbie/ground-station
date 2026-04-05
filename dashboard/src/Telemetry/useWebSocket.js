import { useEffect, useRef, useState } from "react";

export const useWebSocket = (url) => {
  const [lastMessage, setLastMessage] = useState(null);
  const [status, setStatus] = useState("Connecting");
  const socketRef = useRef(null);

  useEffect(() => {
    const socket = new WebSocket(url);
    console.log(`Attempting to connect to ws on: ${url}`);
    socketRef.current = socket;

    socket.onopen = () => setStatus("Open");
    socket.onclose = () => setStatus("Closed");
    socket.onmessage = (event) => setLastMessage(event.data);

    return () => {
      socket.close();
    };
  }, [url]);

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(msg);
    }
  };

  return { lastMessage, status, sendMessage };
};
