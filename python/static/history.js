let allHistory = [];


/* =========================================================
   INITIALIZE
   ========================================================= */

async function initializeHistory() {
    loadCurrentUser();
    await loadHistory();
}


/* =========================================================
   CURRENT USER / THEME
   ========================================================= */

function loadCurrentUser() {
    const button =
        document.getElementById("profile-button");

    const storedUserId =
        localStorage.getItem("inventoryUserId");

    if (!storedUserId) {
        button.textContent = "?";
        return;
    }

    loadUserFromApi(Number(storedUserId));
}


async function loadUserFromApi(userId) {
    try {
        const response = await fetch("/api/users");

        if (!response.ok) {
            return;
        }

        const users = await response.json();

        const user =
            users.find(item => item.id === userId);

        if (!user) {
            return;
        }

        const button =
            document.getElementById("profile-button");

        button.textContent =
            user.name.charAt(0).toUpperCase();

        button.title = user.name;

        if (user.name.toLowerCase() === "jasmine") {
            document.body.classList.add(
                "theme-jasmine"
            );
        }
    }
    catch (error) {
        console.error(error);
    }
}


/* =========================================================
   LOAD / RENDER
   ========================================================= */

async function loadHistory() {
    const response = await fetch("/api/history");

    if (!response.ok) {
        throw new Error("Could not load history.");
    }

    allHistory = await response.json();
    renderHistory(allHistory);
}


function renderHistory(transactions) {
    const container =
        document.getElementById("history");

    const count =
        document.getElementById("history-count");

    container.innerHTML = "";

    count.textContent =
        `${transactions.length} ${
            transactions.length === 1
                ? "change"
                : "changes"
        }`;

    if (transactions.length === 0) {
        container.innerHTML = `
            <p class="loading">
                No inventory history yet.
            </p>
        `;
        return;
    }

    const grouped = new Map();

    for (const transaction of transactions) {
        const date =
            parseUtcTimestamp(
                transaction.created_at
            );

        const key = getDateGroupKey(date);

        if (!grouped.has(key)) {
            grouped.set(key, []);
        }

        grouped.get(key).push({
            ...transaction,
            localDate: date
        });
    }

    for (const [dateLabel, entries] of grouped) {
        const group =
            document.createElement("section");

        group.className = "history-day";

        const heading =
            document.createElement("h3");

        heading.className =
            "history-day-heading";

        heading.textContent = dateLabel;
        group.appendChild(heading);

        const card =
            document.createElement("div");

        card.className = "history-card";

        for (const entry of entries) {
            card.appendChild(
                createHistoryRow(entry)
            );
        }

        group.appendChild(card);
        container.appendChild(group);
    }
}


function createHistoryRow(transaction) {
    const row = document.createElement("div");
    row.className = "history-item";

    const presentation =
        getTransactionPresentation(transaction);

    const time =
        transaction.localDate.toLocaleTimeString(
            [],
            {
                hour: "numeric",
                minute: "2-digit"
            }
        );

    row.innerHTML = `
        <div
            class="history-icon
            ${presentation.iconClass}"
        >
            ${presentation.icon}
        </div>

        <div class="history-details">
            <div class="history-title-row">
                <p class="history-product">
                    ${escapeHtml(transaction.product)}
                </p>

                <span class="history-quantity">
                    ${escapeHtml(
                        presentation.quantityLabel
                    )}
                </span>
            </div>

            <p class="history-action">
                ${presentation.actionHtml}
            </p>

            <p class="history-meta">
                ${escapeHtml(transaction.user)}
                ·
                ${time}
            </p>
        </div>
    `;

    return row;
}


function getTransactionPresentation(transaction) {
    const type =
        transaction.transaction_type ||
        (
            transaction.quantity_change > 0
                ? "restock"
                : "consume"
        );

    const quantity =
        Math.abs(transaction.quantity_change);

    const unit =
        pluralize(transaction.unit, quantity);

    if (type === "move") {
        return {
            icon: "⇄",
            iconClass: "history-move",
            quantityLabel: `${quantity}`,
            actionHtml:
                `Moved ${quantity} ${escapeHtml(unit)}` +
                ` · ${escapeHtml(transaction.location)}` +
                ` → ${escapeHtml(
                    transaction.destination_location || ""
                )}`
        };
    }

    if (type === "adjustment") {
        const before =
            transaction.previous_quantity;

        const after =
            transaction.new_quantity;

        return {
            icon: "✓",
            iconClass: "history-adjust",
            quantityLabel:
                transaction.quantity_change > 0
                    ? `+${transaction.quantity_change}`
                    : `${transaction.quantity_change}`,
            actionHtml:
                `Counted · ${escapeHtml(transaction.location)}` +
                (
                    before !== null &&
                    after !== null
                        ? `<br><span class="history-count-change">` +
                          `${before} → ${after} ` +
                          `${escapeHtml(
                              pluralize(
                                  transaction.unit,
                                  after
                              )
                          )}</span>`
                        : ""
                )
        };
    }

    if (type === "restock") {
        return {
            icon: "+",
            iconClass: "history-add",
            quantityLabel: `+${quantity}`,
            actionHtml:
                `Restocked ${quantity} ${escapeHtml(unit)}` +
                ` · ${escapeHtml(transaction.location)}`
        };
    }

    return {
        icon: "−",
        iconClass: "history-remove",
        quantityLabel: `−${quantity}`,
        actionHtml:
            `Consumed ${quantity} ${escapeHtml(unit)}` +
            ` · ${escapeHtml(transaction.location)}`
    };
}


/* =========================================================
   DATES / TEXT
   ========================================================= */

function parseUtcTimestamp(timestamp) {
    return new Date(
        timestamp.replace(" ", "T") + "Z"
    );
}


function getDateGroupKey(date) {
    const now = new Date();
    const today = startOfDay(now);
    const transactionDay = startOfDay(date);

    const difference =
        Math.round(
            (today - transactionDay) /
            86400000
        );

    if (difference === 0) {
        return "Today";
    }

    if (difference === 1) {
        return "Yesterday";
    }

    return date.toLocaleDateString(
        [],
        {
            month: "long",
            day: "numeric",
            year:
                date.getFullYear() !==
                now.getFullYear()
                    ? "numeric"
                    : undefined
        }
    );
}


function startOfDay(date) {
    return new Date(
        date.getFullYear(),
        date.getMonth(),
        date.getDate()
    );
}


function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


function escapeHtml(value) {
    const element =
        document.createElement("div");

    element.textContent = value ?? "";
    return element.innerHTML;
}


/* =========================================================
   NAVIGATION / START
   ========================================================= */

document
    .getElementById("nav-home-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/";
        }
    );

document
    .getElementById("nav-scan-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/scan";
        }
    );

initializeHistory()
    .catch(error => {
        console.error(error);

        document
            .getElementById("history")
            .innerHTML = `
                <p class="loading">
                    Could not load history.
                </p>
            `;
    });
