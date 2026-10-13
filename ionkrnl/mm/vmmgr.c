/**
 * @file ionkrnl/mm/vmmgr.c
 * @brief Virtual memory manager
 * 
 * This handles managing all the memory in Ion (kernel + userspace)
 * @todo It will be rbtree'd at one point or the other
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ion.h>

/* Kernel arena */
MM_ARENA __MmKernelArena;
MM_ARENA *MmKernelArena = &__MmKernelArena;

// Because arenas are the only level above raw PFN allocation they need to have their nodes
// allocated in a different way. For now, the HHDM is used, but this should be replaced by something better.
typedef struct _MM_ARENA_NODE_PAGE {
    struct _MM_ARENA_NODE_PAGE *Next;
    LIST_ENTRY Entry;
    SIZE_T Active;
    UINT64 Bitmap[4]; // up to 256 nodes per page
    MM_ARENA_NODE Nodes[];
} MM_ARENA_NODE_PAGE;

#define NODES_PER_PAGE ((PAGE_SIZE - (sizeof(MM_ARENA_NODE_PAGE))) / sizeof(MM_ARENA_NODE))

// Mini slab allocator
LIST_ENTRY ArenaFullPages;
LIST_ENTRY ArenaPartialPages;
LIST_ENTRY ArenaEmptyPages;
KSPIN_LOCK ArenaPageLock;

// Helper to allocate a new node
static MM_ARENA_NODE *MmAllocateArenaNode() {
    KIRQL OldIrql;
    KeAcquireSpinLock(&ArenaPageLock, &OldIrql);

    // Try empty then partial
    MM_ARENA_NODE_PAGE *TargetPage = NULL;
    if (!IsListEmpty(&ArenaEmptyPages)) {
        TargetPage = CONTAINING_RECORD(RemoveHeadList(&ArenaEmptyPages), MM_ARENA_NODE_PAGE, Entry);
    } else if (!IsListEmpty(&ArenaPartialPages)) {
        TargetPage = CONTAINING_RECORD(RemoveHeadList(&ArenaPartialPages), MM_ARENA_NODE_PAGE, Entry);
    } else {
        KeReleaseSpinLock(&ArenaPageLock, OldIrql);

        // Allocate a new page
        PFN_NUMBER Pfn = MmAllocatePhysicalPage();
        TargetPage = (MM_ARENA_NODE_PAGE*)MiConvertToHhdm(MmConvertPfnToPhys(Pfn));
        RtlZeroMemory(TargetPage, sizeof(MM_ARENA_NODE_PAGE));

        KeAcquireSpinLock(&ArenaPageLock, &OldIrql);
    }

    // Allocate from it
    MM_ARENA_NODE *AllocNode = NULL;
    for (SIZE_T i = 0; i < 4 && i * 64 < NODES_PER_PAGE; i++) {
        SIZE_T Remaining = NODES_PER_PAGE - i * 64;
        UINT64 ValidBits = Remaining >= 64 ? ~0ULL : (1ULL << Remaining) - 1;
        UINT64 Bitmask = ~TargetPage->Bitmap[i] & ValidBits;
        if (Bitmask == 0) continue;
        
        INT Bit = __builtin_ctzll(Bitmask);
        TargetPage->Bitmap[i] |= (1ULL << Bit);
        TargetPage->Active += 1;

        AllocNode = &TargetPage->Nodes[Bit + (i * 64)];
        break;
    }

    if (AllocNode == NULL) {
        KeBugCheckWithMessage("Corrupt arena node page list, found page without anything free in wrong list?");
    }

    // Reinsert the page
    if (TargetPage->Active >= NODES_PER_PAGE) {
        InsertTailList(&ArenaFullPages, &TargetPage->Entry);
    } else {
        InsertTailList(&ArenaPartialPages, &TargetPage->Entry);
    }

    KeReleaseSpinLock(&ArenaPageLock, OldIrql);
    return AllocNode;
}

// Helper to free an arena node
static VOID MmFreeArenaNode(MM_ARENA_NODE *Node) {
    MM_ARENA_NODE_PAGE *Page = (MM_ARENA_NODE_PAGE*)((UINT_PTR)Node & ~(PAGE_SIZE-1));

    KIRQL Irql;
    KeAcquireSpinLock(&ArenaPageLock, &Irql);
    RemoveEntryList(&Page->Entry);

    Page->Active -= 1;
    
    INT Index = (((UINT_PTR)Node & (PAGE_SIZE-1)) - sizeof(MM_ARENA_NODE_PAGE)) / sizeof(MM_ARENA_NODE);

    ASSERT(Page->Bitmap[Index / 64] & (1ULL << (Index % 64)));
    Page->Bitmap[Index / 64] &= ~(1ULL << (Index % 64));
    
    if (Page->Active == 0) {
        InsertTailList(&ArenaEmptyPages, &Page->Entry);
    } else {
        InsertTailList(&ArenaPartialPages, &Page->Entry);
    }

    KeReleaseSpinLock(&ArenaPageLock, Irql);
}

// Basic BST functions
// TODO This will be upgraded to an AVL tree later on, but for now it is worst case O(n)

static VOID MmInsertBst(MM_ARENA *Arena, MM_ARENA_NODE *Node) {
    MM_ARENA_NODE **Link = &Arena->Root;
    MM_ARENA_NODE *Parent = NULL;

    while (*Link) {
        Parent = *Link;

        if (Node->StartPfn < Parent->StartPfn) {
            Link = &Parent->Left;
        } else {
            Link = &Parent->Right;
        }
    }

    *Link = Node;
    Node->Parent = Parent;
    Node->Left = NULL;
    Node->Right = NULL;

    if (Arena->Leftmost == NULL || Node->StartPfn < Arena->Leftmost->StartPfn) {
        Arena->Leftmost = Node;
    }
}

static VOID MmReplaceBst(MM_ARENA *Arena, MM_ARENA_NODE *Old, MM_ARENA_NODE *New) {
    if (!Old->Parent) {
        Arena->Root = New;
    } else if (Old == Old->Parent->Left) {
        Old->Parent->Left = New;
    } else {
        Old->Parent->Right = New;
    }

    if (New) {
        New->Parent = Old->Parent;
    }
}

static MM_ARENA_NODE *MmLeftmostBst(MM_ARENA_NODE *Node) {
    while (Node && Node->Left) {
        Node = Node->Left;
    }

    return Node;
}

static MM_ARENA_NODE *MmNextBst(MM_ARENA_NODE *Node) {
    if (Node->Right) {
        return MmLeftmostBst(Node->Right);
    }

    while (Node->Parent && Node == Node->Parent->Right) {
        Node = Node->Parent;
    }

    return Node->Parent;
}

static VOID MmRemoveBst(MM_ARENA *Arena, MM_ARENA_NODE *Node) {
    if (Arena->Leftmost == Node) {
        Arena->Leftmost = MmNextBst(Node);
    }

    if (Node->Left == NULL) {
        MmReplaceBst(Arena, Node, Node->Right);
    } else if (Node->Right == NULL) {
        MmReplaceBst(Arena, Node, Node->Left);
    } else {
        MM_ARENA_NODE *Successor = MmLeftmostBst(Node->Right);

        if (Successor->Parent != Node) {
            MmReplaceBst(Arena, Successor, Successor->Right);

            Successor->Right = Node->Right;
            Successor->Right->Parent = Successor;
        }

        MmReplaceBst(Arena, Node, Successor);
        
        Successor->Left = Node->Left;
        Successor->Left->Parent = Successor;
    }

    Node->Left = Node->Right = Node->Parent = NULL;
}

static MM_ARENA_NODE *MmFindContainingFreeRange(MM_ARENA *Arena, PFN_NUMBER StartPage, SIZE_T NumPages) {
    MM_ARENA_NODE *Node = Arena->Root;
    while (Node) {
        PFN_NUMBER Start = Node->StartPfn;
        SIZE_T End = Start + Node->NumPages;

        if (StartPage < Start) {
            Node = Node->Left;
        } else if (StartPage >= End) {
            Node = Node->Right;
        } else {
            // StartPage lies within this
            if (NumPages <= End - StartPage) {
                return Node;
            } else {
                return NULL;
            }
        }
    }

    return NULL;
}

static OS_STATUS MmAllocateArenaAt(MM_ARENA *Arena, PFN_NUMBER StartPage, SIZE_T NumPages, MM_ARENA_NODE **Spare, MM_ARENA_NODE **Removed) {
    MM_ARENA_NODE *Node = MmFindContainingFreeRange(Arena, StartPage, NumPages);
    if (!Node) {
        return STATUS_NO_MEMORY;
    }

    PFN_NUMBER FreeStart = Node->StartPfn;
    PFN_NUMBER FreeEnd = FreeStart + Node->NumPages;
    PFN_NUMBER AllocEnd = StartPage + NumPages;

    if (StartPage == FreeStart && AllocEnd == FreeEnd) {
        // Case 1: Alloc fits entirely in range
        MmRemoveBst(Arena, Node);    
        *Removed = Node;
    } else if (StartPage == FreeStart) {
        // Case 2: From beginning
        MmRemoveBst(Arena, Node);
        Node->StartPfn = AllocEnd;
        Node->NumPages = FreeEnd - AllocEnd;
        MmInsertBst(Arena, Node);
    } else if (AllocEnd == FreeEnd) {
        // Case 3: Alloc from end
        Node->NumPages = StartPage - FreeStart;
    } else {
        // Case 4: Alloc from middle
        MM_ARENA_NODE *NewNode = *Spare;
        Node->NumPages = StartPage - FreeStart;
        NewNode->StartPfn = AllocEnd;
        NewNode->NumPages = FreeEnd - AllocEnd;
        *Spare = NULL;
        MmInsertBst(Arena, NewNode);
    }

    return STATUS_SUCCESS;
}

static OS_STATUS MmFreeArenaAt(MM_ARENA *Arena, PFN_NUMBER StartPage, SIZE_T NumPages, MM_ARENA_NODE **Spare, MM_ARENA_NODE **Removed) {
    PFN_NUMBER EndPage = StartPage + NumPages;
    
    MM_ARENA_NODE *Prev = NULL;
    MM_ARENA_NODE *Next = NULL;
    MM_ARENA_NODE *Node = Arena->Root;

    while (Node) {
        if (StartPage < Node->StartPfn) {
            Next = Node;
            Node = Node->Left;
        } else {
            Prev = Node;
            Node = Node->Right;
        }
    }

    if (Prev && StartPage - Prev->StartPfn < Prev->NumPages) {
        DbgPrint("Warning: MmFreeArenaAt bad free StartPage=0x%x NumPages=%zu conflict with existing StartPage=0x%x NumPages=%zu\n", StartPage, NumPages, Prev->StartPfn, Prev->NumPages);
        return STATUS_INVALID_PARAMETER;
    }

    if (Next && EndPage > Next->StartPfn) {
        DbgPrint("Warning: MmFreeArenaAt bad free EndPage 0x%x overlaps StartPfn 0x%x\n", EndPage, Next->StartPfn);
        return STATUS_INVALID_PARAMETER;
    }

    BOOLEAN MergeLeft = Prev && Prev->StartPfn + Prev->NumPages == StartPage;
    BOOLEAN MergeRight = Next && EndPage == Next->StartPfn;

    if (MergeLeft && MergeRight) {
        // Join them all together
        Prev->NumPages += NumPages + Next->NumPages;
        MmRemoveBst(Arena, Next);
        *Removed = Next;
    } else if (MergeLeft) {
        // Extend
        Prev->NumPages += NumPages;
    } else if (MergeRight) {
        MmRemoveBst(Arena, Next);

        Next->StartPfn = StartPage;
        Next->NumPages += NumPages;

        MmInsertBst(Arena, Next);
    } else {
        MM_ARENA_NODE *NewNode = *Spare;
        *Spare = NULL;

        NewNode->StartPfn = StartPage;
        NewNode->NumPages = NumPages;

        MmInsertBst(Arena, NewNode);
    }

    return STATUS_SUCCESS;
}

// Initialize the kernel arena
OS_STATUS MiInitializeKernelArena() {
    KeInitializeSpinLock(&ArenaPageLock);
    InitializeListHead(&ArenaFullPages);
    InitializeListHead(&ArenaEmptyPages);
    InitializeListHead(&ArenaPartialPages);

    KeInitializeSpinLock(&MmKernelArena->SpinLock);
    MmKernelArena->Base = MM_KERNEL_START;
    MmKernelArena->TotalPages = (MM_KERNEL_END - MM_KERNEL_START) / PAGE_SIZE;
    MmKernelArena->Root = NULL;

    // Need to initialize the base free area
    MM_ARENA_NODE *Node = MmAllocateArenaNode();
    Node->StartPfn = 0;
    Node->NumPages = MmKernelArena->TotalPages;
    Node->Left = Node->Right = Node->Parent = NULL;
    MmInsertBst(MmKernelArena, Node);

    DbgPrint("MiInitializeKernelArena successful\n");

    return STATUS_SUCCESS;
}

// Allocate virtual pages from an arena
OS_STATUS MmAllocateVirtualPagesArenaEx(
    MM_ARENA *Arena,
    VOID *AddressHint,
    SIZE_T NumPages,
    UINT Flags,
    UINT Protection,
    VOID **AddressOut
)
{
    if (Flags & (MM_ALLOC_FIXED_REPLACE)) {
        KeBugCheckWithMessage("MM_ALLOC_FIXED_REPLACE NYI\n");
    }

    if ((UINT_PTR)AddressHint & (PAGE_SIZE-1)) {
        return STATUS_INVALID_PARAMETER;
    }

    BOOLEAN Fixed = !!(Flags & MM_ALLOC_FIXED);
    if (Fixed && (AddressHint == NULL || AddressHint < Arena->Base || AddressHint >= Arena->Base + (Arena->TotalPages*PAGE_SIZE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (NumPages >= Arena->TotalPages) {
        return STATUS_NO_MEMORY;
    }

    // !!! We need to have a spare node ready in case
    // This is bad, but after running the numbers its not horrible.
    // On initialization this spare node is practically guranteed to be used since the entire range is one
    // big block, and only under severe fragmentation does freeing become a challenge.
    MM_ARENA_NODE *SpareNode = MmAllocateArenaNode();
    MM_ARENA_NODE *Removed = NULL;

    // Lock the arena
    OS_STATUS Status = STATUS_SUCCESS;
    KIRQL Irql;
    KeAcquireSpinLock(&Arena->SpinLock, &Irql);

    // Check address hint
    PFN_NUMBER StartPage;
    if (AddressHint != NULL) {
        PFN_NUMBER HintPage = MmConvertPhysToPfn((UINT_PTR)AddressHint - Arena->Base);
        
        if (NumPages > Arena->TotalPages - HintPage) {
            Status = STATUS_INVALID_PARAMETER;
            goto _Exit;
        }

        StartPage = HintPage;
    } else {
        // First-fit allocation
        MM_ARENA_NODE *Node = Arena->Leftmost;
        while (Node && Node->NumPages < NumPages) {
            Node = MmNextBst(Node);
        }

        if (!Node) {
            Status = STATUS_NO_MEMORY;
            goto _Exit;
        }

        StartPage = Node->StartPfn;
    }

    Status = MmAllocateArenaAt(
        Arena,
        StartPage,
        NumPages,
        &SpareNode,
        &Removed
    );

_Exit:
    KeReleaseSpinLock(&Arena->SpinLock, Irql);

    if (Removed != NULL) {
        MmFreeArenaNode(Removed);
    }

    if (SpareNode != NULL) {
        MmFreeArenaNode(SpareNode);
    }

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    UINT_PTR Address = (Arena->Base + StartPage * PAGE_SIZE);
    *AddressOut = (VOID*)(Address);

    // If the allocation is unbacked, finish now.
    if ((Flags & MM_ALLOC_UNBACKED)) {
        return STATUS_SUCCESS;
    }

    // TODO: On userspace allocations, some of this will have to be demand-paged in
    for (UINT_PTR i = Address; i < Address + NumPages * PAGE_SIZE; i += PAGE_SIZE) {
        PFN_NUMBER Frame;
        
        if ((Flags & MM_ALLOC_UNINIT)) {
            Frame = MmAllocatePhysicalPage();
        } else {
            Frame = MmAllocatePhysicalPageZeroed();
        }

        MmuMapPage(
            MmuCurrentPageTable(),
            i,
            MmConvertPfnToPhys(Frame),
            Protection
        );

        // The virtual address may still have a translation from an earlier allocation.
        MmuInvalidateRange(i, 1);
    }

    return STATUS_SUCCESS;
}


// WARNING: YOU MUST ONLY CALL MmFreeVirtualPagesArena WITH BACKED MEMORY
// IT WILL FREE THE BACKED MEMORY!
OS_STATUS MmFreeVirtualPagesArena(
    MM_ARENA *Arena,
    VOID *Address,
    SIZE_T NumPages
)
{
    if ((UINT_PTR)Address & (PAGE_SIZE-1)) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((UINT_PTR)Address < Arena->Base) {
        return STATUS_INVALID_PARAMETER;
    }

    PFN_NUMBER StartPage = MmConvertPhysToPfn((UINT_PTR)Address - Arena->Base);
    if (StartPage + NumPages > Arena->TotalPages) {
        return STATUS_INVALID_PARAMETER;
    }

    // !!! This is bad and its worse than MmAllocateVirtualPagesArenaEx
    // Most of the time, a free range can be merged into a previous range
    // It's possible to make this better by having a retry functionality, where
    // MmFreeArenaAt returns a magic status that signals for us to do allocation.
    MM_ARENA_NODE *SpareNode = MmAllocateArenaNode();
    MM_ARENA_NODE *RemovedNode = NULL;
    
    // Free all backed memory now
    UINT_PTR PageTable = MmuCurrentPageTable();
    for (UINT_PTR i = (UINT_PTR)Address; i < (UINT_PTR)Address + NumPages * PAGE_SIZE; i += PAGE_SIZE) {
        UINT_PTR Frame = MmuGetPhysicalAddress(PageTable, i);
        MmuUnmapPage(PageTable, i);
        if (Frame != 0x0) {
            MmFreePhysicalPage(MmConvertPhysToPfn(Frame));
        }
    }

    // TLB invalidation
    // TODO: Replace with call to MmInvalidate when ready
    MmuInvalidateRange((UINT_PTR)Address, NumPages);

    // Free the vmem back to the arena
    KIRQL Irql;
    OS_STATUS Status;
    KeAcquireSpinLock(&Arena->SpinLock, &Irql);
    Status = MmFreeArenaAt(Arena, StartPage, NumPages, &SpareNode, &RemovedNode);
    KeReleaseSpinLock(&Arena->SpinLock, Irql);

    if (SpareNode) {
        MmFreeArenaNode(SpareNode);
    }

    if (RemovedNode) {
        MmFreeArenaNode(RemovedNode);
    }

    return Status;
}
