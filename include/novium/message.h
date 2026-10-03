#ifndef NOVIUM_IPC_MESSAGE_H
#define NOVIUM_IPC_MESSAGE_H

#include "ipc.h"

/*
 * Message passing over ports.
 *
 * A message is a fixed size value: a length and IpcMessageWords of payload. It
 * is copied by value rather than referenced, so the sender's buffer is free
 * again as soon as IpcSend returns and neither side has to keep the other's
 * memory alive.
 */

#define IpcMessageWords 8

typedef struct IpcMessage {
	u32 Length; /* words of Data in use, at most IpcMessageWords */
	u32 Sender; /* task id of the sender, filled in by IpcSend */
	u32 Data[IpcMessageWords];
} IpcMessage;

/* How many messages a port holds before senders have to wait for a receiver. */
#define IpcPortCapacity 16

/*
 * A port is a bounded queue of messages in arrival order. What it adds over a
 * plain queue is the other half: a sender that finds the port full and a
 * receiver that finds it empty both park until the other side moves. Port
 * fields are private, build one with IpcPortCreate and do not touch it after.
 */
typedef struct IpcPort {
	u32 Magic;
	u32 Head;   /* slot the next receive takes from */
	u32 Count;  /* messages queued right now */
	IpcMessage Slots[IpcPortCapacity];
	IpcWait Senders;
	IpcWait Receivers;
} IpcPort;

/*
 * Claims a port the caller owns, in its own memory, and empties it. The port
 * must not be in use already, which reports IpcErrInvalid rather than wiping
 * a queue another task is reading from.
 */
IpcStatus IpcPortCreate(IpcPort *Port);

/* Empties the port and releases it. Fails while tasks are parked on it, since
 * dropping them would strand them. */
IpcStatus IpcPortDestroy(IpcPort *Port);

/* Queues a message, waiting for room while the port is full. */
IpcStatus IpcSend(IpcPort *Port, const IpcMessage *Message);

/* Queues a message only when there is room. Never waits. */
IpcStatus IpcTrySend(IpcPort *Port, const IpcMessage *Message);

/* Takes the oldest queued message, waiting while the port is empty. */
IpcStatus IpcReceive(IpcPort *Port, IpcMessage *out);

/* Takes the oldest message only when one is queued. Never waits. */
IpcStatus IpcTryReceive(IpcPort *Port, IpcMessage *out);

/* Messages waiting in the port. */
u32 IpcPortQueued(const IpcPort *Port);

/* True once IpcPortCreate has claimed the port. */
bool IpcPortIsCreated(const IpcPort *Port);

#endif
