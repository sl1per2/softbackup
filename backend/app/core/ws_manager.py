from fastapi import WebSocket
from typing import Dict, Set
import asyncio
import json


class WebSocketManager:
    def __init__(self):
        self.channels: Dict[str, Set[WebSocket]] = {}

    async def connect(self, websocket: WebSocket, channel: str):
        await websocket.accept()
        if channel not in self.channels:
            self.channels[channel] = set()
        self.channels[channel].add(websocket)

    def disconnect(self, websocket: WebSocket, channel: str):
        if channel in self.channels:
            self.channels[channel].discard(websocket)

    async def broadcast(self, channel: str, message: dict):
        if channel not in self.channels:
            return
        dead = set()
        for ws in self.channels[channel]:
            try:
                await ws.send_json(message)
            except Exception:
                dead.add(ws)
        self.channels[channel] -= dead

    async def broadcast_job_update(self, job_id: str, status: str, progress: float):
        await self.broadcast("jobs", {
            "type": "job_update",
            "job_id": job_id,
            "status": status,
            "progress": progress,
        })
        await self.broadcast("dashboard", {
            "type": "job_update",
            "job_id": job_id,
            "status": status,
            "progress": progress,
        })

    async def broadcast_cdp_metrics(self, session_id: str, metrics: dict):
        await self.broadcast("cdp", {
            "type": "cdp_metrics",
            "session_id": session_id,
            **metrics,
        })

    async def broadcast_traffic_stats(self, stats: dict):
        await self.broadcast("traffic", {
            "type": "traffic_stats",
            **stats,
        })


ws_manager = WebSocketManager()
