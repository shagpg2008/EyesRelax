#pragma once

// Requests an immediate operating-system shutdown. Returns true when the
// request was accepted; the actual shutdown may still be subject to OS policy.
bool RequestSystemShutdown();
