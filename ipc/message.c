#include <novium/cpu.h>
#include <novium/message.h>

#define PORT_MAGIC 0x504F5254u /* ASCII "PORT", tells a real port from garbage */

/* Drops whatever is in the slots and wait queues, keeping the port claimed. */
static void PortReset(IpcPort *Port) {
	u32 Index;

	Port->Head = 0;
	Port->Count = 0;

	for (Index = 0; Index < IpcPortCapacity; Index++) {
		Port->Slots[Index].Length = 0;
		Port->Slots[Index].Sender = 0;
	}

	Port->Senders.Count = 0;
	Port->Receivers.Count = 0;
}

bool IpcPortIsCreated(const IpcPort *Port) {
	return Port != 0 && Port->Magic == PORT_MAGIC;
}

IpcStatus IpcPortCreate(IpcPort *Port) {
	bool was_enabled;

	if (Port == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Port->Magic == PORT_MAGIC) {
		/* Already in use. Wiping it here would empty a queue under a reader. */
		if (was_enabled) {
			cpu_enable_irqs();
		}
		return IpcErrInvalid;
	}

	Port->Magic = PORT_MAGIC;
	PortReset(Port);

	if (was_enabled) {
		cpu_enable_irqs();
	}

	return IpcOk;
}

IpcStatus IpcPortDestroy(IpcPort *Port) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (!IpcPortIsCreated(Port)) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Port->Senders.Count != 0 || Port->Receivers.Count != 0) {
		/*
		 * Parked tasks would never learn the port went away, so a busy port
		 * stays claimed and the caller has to drain it first.
		 */
		Status = IpcErrBusy;
	} else {
		PortReset(Port);
		Port->Magic = 0;
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	return Status;
}

u32 IpcPortQueued(const IpcPort *Port) {
	if (!IpcPortIsCreated(Port)) {
		return 0;
	}

	return Port->Count;
}

/* True while the port has room, which is what a blocked sender is waiting for. */
static bool PortHasRoom(void *arg) {
	IpcPort *Port = (IpcPort *)arg;

	return Port->Count < IpcPortCapacity;
}

/* True while the port holds something, which is what a receiver waits for. */
static bool PortHasMessage(void *arg) {
	IpcPort *Port = (IpcPort *)arg;

	return Port->Count > 0;
}

/* Copies the message into the next free slot. Caller holds the lock. */
static void PortPush(IpcPort *Port, const IpcMessage *Message) {
	u32 Tail = (Port->Head + Port->Count) % IpcPortCapacity;

	Port->Slots[Tail] = *Message;
	Port->Slots[Tail].Sender = IpcSelfId();
	Port->Count++;
}

/* Copies the oldest message out. Caller holds the lock. */
static void PortPop(IpcPort *Port, IpcMessage *out) {
	*out = Port->Slots[Port->Head];
	Port->Head = (Port->Head + 1) % IpcPortCapacity;
	Port->Count--;
}

IpcStatus IpcTrySend(IpcPort *Port, const IpcMessage *Message) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (!IpcPortIsCreated(Port) || Message == 0) {
		return IpcErrInvalid;
	}

	if (Message->Length > IpcMessageWords) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Port->Count == IpcPortCapacity) {
		Status = IpcErrFull;
	} else {
		PortPush(Port, Message);
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (Status == IpcOk) {
		IpcWakeOne(&Port->Receivers);
	}

	return Status;
}

IpcStatus IpcSend(IpcPort *Port, const IpcMessage *Message) {
	if (!IpcPortIsCreated(Port) || Message == 0) {
		return IpcErrInvalid;
	}

	if (Message->Length > IpcMessageWords) {
		return IpcErrInvalid;
	}

	for (;;) {
		IpcStatus Status = IpcTrySend(Port, Message);

		if (Status != IpcErrFull) {
			return Status;
		}

		/* Full. Wait for a receiver to make room, then try again. */
		Status = IpcPark(&Port->Senders, PortHasRoom, Port);
		if (Status != IpcOk) {
			return Status;
		}
	}
}

IpcStatus IpcTryReceive(IpcPort *Port, IpcMessage *out) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (!IpcPortIsCreated(Port) || out == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Port->Count == 0) {
		Status = IpcErrEmpty;
	} else {
		PortPop(Port, out);
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (Status == IpcOk) {
		IpcWakeOne(&Port->Senders);
	}

	return Status;
}

IpcStatus IpcReceive(IpcPort *Port, IpcMessage *out) {
	if (!IpcPortIsCreated(Port) || out == 0) {
		return IpcErrInvalid;
	}

	for (;;) {
		IpcStatus Status = IpcTryReceive(Port, out);

		if (Status != IpcErrEmpty) {
			return Status;
		}

		/* Empty. Wait for a sender, then try again. */
		Status = IpcPark(&Port->Receivers, PortHasMessage, Port);
		if (Status != IpcOk) {
			return Status;
		}
	}
}
