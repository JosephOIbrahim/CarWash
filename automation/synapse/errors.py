"""Typed exceptions for the Synapse bridge layer.

Constitution: C2 — errors are loud. Replaces the `except: pass` pattern with
specific types callers can catch when they truly intend to recover.
"""

from __future__ import annotations


class SynapseError(Exception):
    """Base class for all Synapse-bridge failures."""


class SynapseConnectionError(SynapseError):
    """Raised when the bridge cannot be reached (refused, DNS, TLS, etc.)."""


class SynapseTimeoutError(SynapseError):
    """Raised when a request exceeds its deadline."""


class SynapseCommandError(SynapseError):
    """Raised when the bridge returns ``success=False`` for a command.

    Attributes
    ----------
    command:
        The original command payload that was sent.
    response:
        The decoded response object (typically containing an ``error`` field).
    """

    def __init__(
        self,
        message: str,
        *,
        command: object | None = None,
        response: object | None = None,
    ) -> None:
        super().__init__(message)
        self.command = command
        self.response = response
