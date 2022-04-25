let pins = {}

function init() {
	let pin_states = document.getElementsByClassName('state')
	for (let i = 0; i < pin_states.length; i++) {
		let pin = pin_states[i]
		let number = Number(pin.querySelector('output[name="pin"]').innerText)
		pins[number] = pin
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
					pin_html.querySelector('output[name="name"]').innerText = entry.name
					pin_html.querySelector('output[name="resistor"]').innerText = entry.pull_up == true ? 'Pull Up' : 'Pull Down'
					pin_html.querySelector('output[name="state"]').innerText = entry.state
					pin_html.querySelector('output[name="state"]').className = entry.state + "color"
					pin_html.querySelector('output[name="changes"]').innerText = entry.changes
				} else if (!(pin in pins) && !missing.includes(pin)) {
					missing.push(pin)
				} else {
					console.log('Error:', 'Pins on index page or pins in pins json contain a duplicate!')
				}
			}

			if (missing.length > 0) {
				window.location.href = window.location.href
			}
		}).catch((error) => {
			console.log('Error:', error)
		})
}

window.onload = init
