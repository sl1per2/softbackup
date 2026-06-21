from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response, JSONResponse
from app.core.security import get_redis
import time


class RateLimitMiddleware(BaseHTTPMiddleware):
    def __init__(self, app, default_limit: int = 100, window_seconds: int = 60):
        super().__init__(app)
        self.default_limit = default_limit
        self.window_seconds = window_seconds

    async def dispatch(self, request: Request, call_next) -> Response:
        client_ip = request.client.host if request.client else "unknown"
        path = request.url.path

        if path.startswith("/ws") or path == "/health":
            return await call_next(request)

        try:
            r = await get_redis()
            key = f"ratelimit:{client_ip}:{int(time.time()) // self.window_seconds}"
            current = await r.incr(key)
            if current == 1:
                await r.expire(key, self.window_seconds)

            if current > self.default_limit:
                return JSONResponse(
                    status_code=429,
                    content={"detail": "Rate limit exceeded"},
                    headers={"Retry-After": str(self.window_seconds)},
                )
        except Exception:
            pass  # If Redis is down, allow requests through

        return await call_next(request)
