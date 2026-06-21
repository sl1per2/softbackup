import asyncio
import json
import socket
import struct
from typing import Optional


class CoreIPCClient:
    def __init__(self, socket_path: str, timeout: float = 30.0):
        self.socket_path = socket_path
        self.timeout = timeout
        self._reader: Optional[asyncio.StreamReader] = None
        self._writer: Optional[asyncio.StreamWriter] = None

    async def connect(self):
        try:
            self._reader, self._writer = await asyncio.open_unix_connection(
                self.socket_path, limit=10 * 1024 * 1024
            )
        except (ConnectionRefusedError, FileNotFoundError):
            return False
        return True

    async def close(self):
        if self._writer:
            try:
                self._writer.close()
                await self._writer.wait_closed()
            except Exception:
                pass

    async def send_command(self, command_type: int, payload: dict, correlation_id: str = "") -> dict:
        if not self._writer:
            if not await self.connect():
                return {"success": False, "error": "Cannot connect to core"}

        request = {
            "type": command_type,
            "payload": json.dumps(payload),
            "correlation_id": correlation_id,
        }

        data = json.dumps(request).encode()
        msg_size = struct.pack("<I", len(data))

        try:
            self._writer.write(msg_size + data)
            await self._writer.drain()

            size_data = await asyncio.wait_for(self._reader.readexactly(4), self.timeout)
            msg_len = struct.unpack("<I", size_data)[0]

            resp_data = await asyncio.wait_for(self._reader.readexactly(msg_len), self.timeout)
            return json.loads(resp_data.decode())
        except (asyncio.TimeoutError, ConnectionError) as e:
            await self.close()
            return {"success": False, "error": str(e)}

    async def start_job(self, config: dict) -> dict:
        return await self.send_command(0, config)

    async def stop_job(self, job_id: str) -> dict:
        return await self.send_command(1, {"job_id": job_id})

    async def get_status(self, job_id: str) -> dict:
        return await self.send_command(2, {"job_id": job_id})

    async def cdp_start(self, config: dict) -> dict:
        return await self.send_command(3, config)

    async def cdp_stop(self, session_id: str) -> dict:
        return await self.send_command(4, {"session_id": session_id})

    async def restore(self, config: dict) -> dict:
        return await self.send_command(5, config)

    async def get_metrics(self) -> dict:
        return await self.send_command(6, {})

    async def get_traffic_stats(self) -> dict:
        return await self.send_command(7, {})


ipc_client: Optional[CoreIPCClient] = None


async def get_ipc_client() -> CoreIPCClient:
    global ipc_client
    if ipc_client is None:
        from app.core.config import settings
        ipc_client = CoreIPCClient(settings.CORE_SOCKET_PATH)
    return ipc_client
