let selectedPackage = null;
let selectedUser = null;
let allLocations = [];


/* =========================================================
   START SCANNER PAGE
   ========================================================= */

async function initializeScanner() {
    await loadCurrentUser();
    await loadLocations();
}


/* =========================================================
   CURRENT USER + THEME
   ========================================================= */

async function loadCurrentUser() {
    const response = await fetch("/api/users");

    if (!response.ok) {
        throw new Error("Could not load users.");
    }

    const users = await response.json();

    const storedUserId =
        localStorage.getItem("inventoryUserId");

    if (storedUserId !== null) {
        selectedUser =
            users.find(
                user =>
                    user.id === Number(storedUserId)
            ) || null;
    }

    /*
     * Carry Jasmine's pink theme onto this page.
     * Devon continues using the default green.
     */
    if (
        selectedUser &&
        selectedUser.name.toLowerCase() === "jasmine"
    ) {
        document.body.classList.add("theme-jasmine");
    }
}


/* =========================================================
   LOCATIONS
   ========================================================= */

async function loadLocations() {
    const response = await fetch("/api/locations");

    if (!response.ok) {
        throw new Error("Could not load locations.");
    }

    allLocations = await response.json();

    const select =
        document.getElementById("restock-location");

    select.innerHTML = "";

    /*
     * For now, only show locations that have children
     * OR that are actual storage locations.
     *
     * We specifically don't want "Home" or "Kitchen"
     * as normal destinations when putting groceries away.
     */
    const storageLocations =
        allLocations.filter(location => {

            const hasChildren =
                allLocations.some(
                    other =>
                        other.parent_id === location.id
                );

            return !hasChildren;
        });

    for (const location of storageLocations) {
        const option =
            document.createElement("option");

        option.value = location.id;
        option.textContent = location.name;

        select.appendChild(option);
    }
}


/* =========================================================
   BARCODE LOOKUP
   ========================================================= */

document
    .getElementById("barcode-form")
    .addEventListener(
        "submit",
        async event => {

            event.preventDefault();

            const input =
                document.getElementById(
                    "barcode-input"
                );

            const error =
                document.getElementById(
                    "scan-error"
                );

            const resultSection =
                document.getElementById(
                    "scan-result"
                );

            const barcode =
                input.value.trim();

            error.textContent = "";
            resultSection.hidden = true;
            selectedPackage = null;

            if (!barcode) {
                error.textContent =
                    "Enter a barcode.";

                return;
            }

            try {
                const response =
                    await fetch(
                        `/api/products/barcode/${encodeURIComponent(barcode)}`
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Product not found."
                    );
                }

                selectedPackage = result;

                showProduct(result);
            }
            catch (err) {
                error.textContent =
                    err.message;
            }
        }
    );


/* =========================================================
   DISPLAY FOUND PRODUCT
   ========================================================= */

function showProduct(product) {
    document
        .getElementById("scan-product")
        .textContent =
            product.product;

    document
        .getElementById("scan-brand")
        .textContent =
            product.brand || "";

    document
        .getElementById(
            "scan-package-quantity"
        )
        .textContent =
            product.package_quantity;

    document
        .getElementById(
            "scan-package-unit"
        )
        .textContent =
            pluralize(
                product.unit,
                product.package_quantity
            );

    document
        .getElementById("restock-button")
        .textContent =
            `Add ${product.package_quantity} ${
                pluralize(
                    product.unit,
                    product.package_quantity
                )
            }`;

    document
        .getElementById("scan-result")
        .hidden = false;
}


/* =========================================================
   RESTOCK
   ========================================================= */

document
    .getElementById("restock-button")
    .addEventListener(
        "click",
        async () => {

            const error =
                document.getElementById(
                    "scan-error"
                );

            if (!selectedPackage) {
                return;
            }

            if (!selectedUser) {
                error.textContent =
                    "Select a user from the home screen first.";

                return;
            }

            const locationId =
                Number(
                    document
                        .getElementById(
                            "restock-location"
                        )
                        .value
                );

            const button =
                document.getElementById(
                    "restock-button"
                );

            button.disabled = true;
            button.textContent =
                "Adding...";

            error.textContent = "";

            try {
                const response =
                    await fetch(
                        "/api/inventory/add",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    barcode:
                                        selectedPackage
                                            .barcode,

                                    location_id:
                                        locationId,

                                    user_id:
                                        selectedUser.id
                                })
                        }
                    );

                const result =
                    await response.json();

                if (!response.ok) {
                    throw new Error(
                        result.error ||
                        "Could not add inventory."
                    );
                }

                /*
                 * Return to the dashboard after
                 * a successful restock.
                 */
                window.location.href = "/";
            }
            catch (err) {
                error.textContent =
                    err.message;

                button.disabled = false;

                button.textContent =
                    `Add ${
                        selectedPackage
                            .package_quantity
                    } ${
                        pluralize(
                            selectedPackage.unit,
                            selectedPackage
                                .package_quantity
                        )
                    }`;
            }
        }
    );


/* =========================================================
   HELPERS
   ========================================================= */

function pluralize(unit, quantity) {
    if (quantity === 1) {
        return unit;
    }

    return `${unit}s`;
}


/* =========================================================
   START
   ========================================================= */

/* =========================================================
   CAMERA
   ========================================================= */

let cameraStream = null;


async function startCamera() {
    const button =
        document.getElementById(
            "start-camera-button"
        );

    const video =
        document.getElementById(
            "camera-preview"
        );

    const error =
        document.getElementById(
            "scan-error"
        );

    error.textContent = "";

    try {
        cameraStream =
            await navigator.mediaDevices.getUserMedia({
                video: {
                    facingMode: {
                        ideal: "environment"
                    }
                },
                audio: false
            });

        video.srcObject = cameraStream;

        button.hidden = true;
    }
    catch (err) {
        console.error(err);

        error.textContent =
            "Could not access camera. Check Safari camera permissions.";
    }
}


document
    .getElementById("start-camera-button")
    .addEventListener(
        "click",
        startCamera
    );

window.addEventListener(
    "pagehide",
    () => {

        if (cameraStream) {
            for (
                const track
                of cameraStream.getTracks()
            ) {
                track.stop();
            }
        }
    }
);

initializeScanner()
    .catch(error => {

        console.error(error);

        document
            .getElementById("scan-error")
            .textContent =
                "Could not initialize scanner.";
    });