let allInventory = [];
let allUsers = [];

let selectedItem = null;
let selectedUser = null;

let consumeQuantity = 1;


/* =========================================================
   INITIAL DATA
   ========================================================= */

async function initializeApp() {
    await loadUsers();
    await loadInventory();
}


/* =========================================================
   USERS
   ========================================================= */

async function loadUsers() {
    const response = await fetch("/api/users");

    if (!response.ok) {
        throw new Error("Could not load users.");
    }

    allUsers = await response.json();

    /*
     * localStorage belongs to this browser/device.
     *
     * Jasmine's phone can remember Jasmine while
     * Devon's phone independently remembers Devon.
     */
    const storedUserId =
        localStorage.getItem("inventoryUserId");

    if (storedUserId !== null) {
        selectedUser =
            allUsers.find(
                user =>
                    user.id === Number(storedUserId)
            ) || null;
    }

    /*
     * If this phone has never selected a user,
     * leave it unselected.
     */
    updateProfileButton();
}


function selectUser(user) {
    selectedUser = user;

    localStorage.setItem(
        "inventoryUserId",
        user.id
    );

    updateProfileButton();
    renderUserOptions();
    closeUserSheet();
}


function updateProfileButton() {
    const button =
        document.getElementById("profile-button");

    /*
     * Remove any existing user theme first.
     */
    document.body.classList.remove(
        "theme-jasmine"
    );

    if (!selectedUser) {
        button.textContent = "?";
        button.title = "Select user";
        return;
    }

    button.textContent =
        selectedUser.name
            .charAt(0)
            .toUpperCase();

    button.title = selectedUser.name;

    /*
     * Apply Jasmine's pink theme.
     *
     * Devon uses the default green theme.
     */
    if (
        selectedUser.name
            .toLowerCase() === "jasmine"
    ) {
        document.body.classList.add(
            "theme-jasmine"
        );
    }
}


function renderUserOptions() {
    const container =
        document.getElementById("user-options");

    container.innerHTML = "";

    for (const user of allUsers) {
        const button =
            document.createElement("button");

        button.className = "user-option";

        if (
            selectedUser &&
            selectedUser.id === user.id
        ) {
            button.classList.add("selected");
        }

        const currentLabel =
            selectedUser &&
            selectedUser.id === user.id
                ? '<span class="user-current">Current</span>'
                : "";

        button.innerHTML = `
            <span class="user-avatar">
                ${user.name.charAt(0).toUpperCase()}
            </span>

            <span class="user-name">
                ${user.name}
            </span>

            ${currentLabel}
        `;

        button.addEventListener(
            "click",
            () => selectUser(user)
        );

        container.appendChild(button);
    }
}


function openUserSheet() {
    renderUserOptions();

    document
        .getElementById("user-overlay")
        .classList.add("open");

    document
        .getElementById("user-sheet")
        .classList.add("open");
}


function closeUserSheet() {
    document
        .getElementById("user-overlay")
        .classList.remove("open");

    document
        .getElementById("user-sheet")
        .classList.remove("open");
}


/* =========================================================
   INVENTORY
   ========================================================= */

async function loadInventory() {
    const response =
        await fetch("/api/inventory");

    if (!response.ok) {
        throw new Error(
            "Could not load inventory."
        );
    }

    allInventory =
        await response.json();

    renderInventory(allInventory);
}


function renderInventory(items) {
    const container =
        document.getElementById("inventory");

    const itemCount =
        document.getElementById("item-count");

    container.innerHTML = "";

    itemCount.textContent =
        `${items.length} ${
            items.length === 1
                ? "item"
                : "items"
        }`;

    if (items.length === 0) {
        container.innerHTML =
            '<p class="loading">' +
            'No inventory found.' +
            '</p>';

        return;
    }

    const grouped = {};

    for (const item of items) {
        if (!grouped[item.location]) {
            grouped[item.location] = [];
        }

        grouped[item.location].push(item);
    }

    for (
        const [location, locationItems]
        of Object.entries(grouped)
    ) {
        const card =
            document.createElement("div");

        card.className = "location-card";


        /* Location header */

        const header =
            document.createElement("div");

        header.className =
            "location-header";

        const title =
            document.createElement("h3");

        title.textContent = location;

        const count =
            document.createElement("span");

        count.textContent =
            `${locationItems.length} ${
                locationItems.length === 1
                    ? "item"
                    : "items"
            }`;

        header.appendChild(title);
        header.appendChild(count);

        card.appendChild(header);


        /* Inventory rows */

        for (const item of locationItems) {
            const row =
                document.createElement("div");

            row.className =
                "inventory-item";

            row.addEventListener(
                "click",
                () => openConsumeSheet(item)
            );

            row.innerHTML = `
                <div>
                    <p class="item-name">
                        ${item.product}
                    </p>

                    <p class="item-location">
                        ${item.location}
                    </p>
                </div>

                <div class="item-quantity">
                    <span class="number">
                        ${item.quantity}
                    </span>

                    <span class="unit">
                        ${pluralize(
                            item.unit,
                            item.quantity
                        )}
                    </span>
                </div>
            `;

            card.appendChild(row);
        }

        container.appendChild(card);
    }
}


/* =========================================================
   TEXT HELPERS
   ========================================================= */

function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


/* =========================================================
   SEARCH
   ========================================================= */

document
    .getElementById("search-input")
    .addEventListener(
        "input",
        event => {

            const search =
                event.target.value
                    .trim()
                    .toLowerCase();

            const filtered =
                allInventory.filter(
                    item =>
                        item.product
                            .toLowerCase()
                            .includes(search) ||

                        item.location
                            .toLowerCase()
                            .includes(search)
                );

            renderInventory(filtered);
        }
    );


/* =========================================================
   PROFILE BUTTON
   ========================================================= */

document
    .getElementById("profile-button")
    .addEventListener(
        "click",
        openUserSheet
    );


document
    .getElementById("user-sheet-close")
    .addEventListener(
        "click",
        closeUserSheet
    );


document
    .getElementById("user-overlay")
    .addEventListener(
        "click",
        closeUserSheet
    );


/* =========================================================
   CONSUME SHEET
   ========================================================= */

function openConsumeSheet(item) {
    selectedItem = item;
    consumeQuantity = 1;

    document
        .getElementById("sheet-product")
        .textContent =
            item.product;

    document
        .getElementById("sheet-location")
        .textContent =
            item.location;

    document
        .getElementById("sheet-stock-number")
        .textContent =
            item.quantity;

    document
        .getElementById("sheet-stock-unit")
        .textContent =
            pluralize(
                item.unit,
                item.quantity
            );

    document
        .getElementById("consume-error")
        .textContent = "";

    updateConsumeControls();

    document
        .getElementById("sheet-overlay")
        .classList.add("open");

    document
        .getElementById("consume-sheet")
        .classList.add("open");
}


function closeConsumeSheet() {
    document
        .getElementById("sheet-overlay")
        .classList.remove("open");

    document
        .getElementById("consume-sheet")
        .classList.remove("open");

    selectedItem = null;
}


function updateConsumeControls() {
    document
        .getElementById("consume-quantity")
        .textContent =
            consumeQuantity;

    const unit =
        selectedItem
            ? pluralize(
                selectedItem.unit,
                consumeQuantity
            )
            : "items";

    document
        .getElementById("consume-button")
        .textContent =
            `Consume ${consumeQuantity} ${unit}`;
}


/* =========================================================
   QUANTITY BUTTONS
   ========================================================= */

document
    .getElementById("quantity-minus")
    .addEventListener(
        "click",
        () => {

            if (consumeQuantity > 1) {
                consumeQuantity--;
                updateConsumeControls();
            }
        }
    );


document
    .getElementById("quantity-plus")
    .addEventListener(
        "click",
        () => {

            if (
                selectedItem &&
                consumeQuantity <
                    selectedItem.quantity
            ) {
                consumeQuantity++;
                updateConsumeControls();
            }
        }
    );


document
    .getElementById("sheet-close")
    .addEventListener(
        "click",
        closeConsumeSheet
    );


document
    .getElementById("sheet-overlay")
    .addEventListener(
        "click",
        closeConsumeSheet
    );


/* =========================================================
   CONSUME INVENTORY
   ========================================================= */

document
    .getElementById("consume-button")
    .addEventListener(
        "click",
        async () => {

            if (!selectedItem) {
                return;
            }

            /*
             * We no longer assume user_id = 1.
             */
            if (!selectedUser) {
                closeConsumeSheet();
                openUserSheet();
                return;
            }

            const button =
                document.getElementById(
                    "consume-button"
                );

            const error =
                document.getElementById(
                    "consume-error"
                );

            button.disabled = true;
            button.textContent =
                "Updating...";

            error.textContent = "";

            try {
                const response =
                    await fetch(
                        "/api/inventory/remove",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    product_id:
                                        selectedItem
                                            .product_id,

                                    location_id:
                                        selectedItem
                                            .location_id,

                                    user_id:
                                        selectedUser.id,

                                    quantity:
                                        consumeQuantity
                                })
                        }
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Could not update inventory."
                    );
                }

                closeConsumeSheet();

                await loadInventory();
            }
            catch (err) {
                error.textContent =
                    err.message;

                button.disabled = false;

                updateConsumeControls();
            }
        }
    );


/* =========================================================
   START APP
   ========================================================= */

initializeApp()
    .catch(error => {

        console.error(error);

        document
            .getElementById("inventory")
            .innerHTML =
                '<p class="loading">' +
                'Could not load inventory.' +
                '</p>';
    });



function openScanner() {
    window.location.href = "/scan";
}


document
    .getElementById("scan-button")
    .addEventListener(
        "click",
        openScanner
    );


document
    .getElementById("nav-scan-button")
    .addEventListener(
        "click",
        openScanner
    );

document
    .getElementById("nav-history-button")
    .addEventListener(
        "click",
        () => {
            window.location.href = "/history";
        }
    );