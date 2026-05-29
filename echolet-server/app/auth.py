from __future__ import annotations

import hmac

from fastapi import Header, HTTPException, status

from app.config import get_api_token


def verify_bearer_token(authorization: str | None = Header(default=None)) -> None:
    if not authorization:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Missing Authorization header",
        )

    scheme, _, token = authorization.partition(" ")
    expected_token = get_api_token()
    is_valid = scheme.lower() == "bearer" and hmac.compare_digest(token, expected_token)

    if not is_valid:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid bearer token",
        )
