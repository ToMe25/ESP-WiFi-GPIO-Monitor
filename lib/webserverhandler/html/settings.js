let pins = {}
let changed = []
let any_changed = false

function init() {
	let pin_states = document.getElementsByClassName('state')
	for (let i = 0; i < pin_states.length; i++) {
		let pin = pin_states[i]

		pin.addEventListener('change', onChange, false)
		pin.name.addEventListener('input', onChange, false)

		if (pin.attributes.name == undefined || pin.attributes.name.value != "add") {
			let number = Number(pin.pin.value)
			pins[number] = pin
		}
	}
	window.setInterval(update, 5000)
}

function update() {
	fetch('pins.json', { method: 'get' })
		.then(res => res.json())
		.then(data => {
			let missing = Object.keys(pins)
			let new_pins = Object.keys(data)
			for (let i = 0; i < new_pins.length; i++) {
				let pin = new_pins[i]
				let entry = data[pin]
				if (pin in pins && missing.includes(pin)) {
					let pin_html = pins[pin]
					missing.splice(missing.indexOf(pin), 1)

					if (!changed.includes(pin)) {
						pin_html.name.value = entry.name
						pin_html.resistor[entry.pull_up == true ? 0 : 1].checked = true
					}

					pin_html.querySelector('output[name="state"]').innerText = entry.state
					pin_html.querySelector('output[name="state"]').className = entry.state + "color"
					pin_html.querySelector('output[name="changes"]').innerText = entry.changes
				} else if (!(pin in pins) && !missing.includes(pin)) {
					missing.push(pin)
				} else {
					console.warn('Error:', 'Pins on index page or pins in pins json contain a duplicate!')
				}
			}

			if (!any_changed && missing.length > 0) {
				window.location.href = window.location.href
			}
		}).catch((error) => {
			console.warn('Error:', error)
		})
}

function onChange(event) {
	any_changed = true
	let form = event.target.parentNode.parentNode

	if (form.attributes.name != undefined && form.attributes.name.value == "add") {
		return
	}

	let pin = form.pin.value
	if (!changed.includes(pin)) {
		changed.push(pin)
	}
}

window.onload = init
