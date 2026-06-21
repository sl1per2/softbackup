from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import Response
from app.core.database import async_session
from sqlalchemy import text
from datetime import datetime, timezone


class AuditMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next) -> Response:
        response = await call_next(request)

        # Skip non-audit paths
        skip_paths = {"/docs", "/openapi.json", "/health", "/ws/"}
        path = request.url.path
        if any(path.startswith(p) for p in skip_paths) or path.endswith("/ws"):
            return response

        try:
            user_id = None
            auth = request.headers.get("Authorization", "")
            if auth.startswith("Bearer "):
                from app.core.security import decode_token
                try:
                    payload = decode_token(auth[7:])
                    user_id = int(payload.get("sub", 0))
                except Exception:
                    pass

            action = f"{request.method} {path}"
            resource_type = path.split("/")[2] if len(path.split("/")) > 2 else "unknown"

            async with async_session() as session:
                await session.execute(
                    text("""INSERT INTO audit_logs (user_id, action, resource_type, details, ip_address, created_at)
                            VALUES (:user_id, :action, :resource_type, :details, :ip, :created_at)"""),
                    {
                        "user_id": user_id,
                        "action": action,
                        "resource_type": resource_type,
                        "details": "{}",
                        "ip": request.client.host if request.client else "unknown",
                        "created_at": datetime.now(timezone.utc),
                    },
                )
        except Exception:
            pass  # Don't fail requests due to audit logging

        return response
