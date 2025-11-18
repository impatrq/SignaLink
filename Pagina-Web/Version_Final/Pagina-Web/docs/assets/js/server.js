// server.js
import express from 'express'
import cors from 'cors'
import dotenv from 'dotenv'
import { Resend } from 'resend'

dotenv.config({ path: '.env.local' })

const app = express()
const port = process.env.PORT || 3000
const resend = new Resend(process.env.RESEND_API_KEY)

app.use(cors())
app.use(express.json())

app.post('/send', async (req, res) => {
  const { name, email, subject, message } = req.body

  try {
    const data = await resend.emails.send({
      from: 'Contacto Web <onboarding@resend.dev>',
      to: 'signalink2025@gmail.com',
      subject: `Mensaje de ${name}: ${subject}`,
      html: `<p><strong>Nombre:</strong> ${name}</p><p><strong>Email:</strong> ${email}</p><p><strong>Mensaje:</strong><br>${message}</p>`
    })

    res.status(200).json({ success: true, id: data.id })
  } catch (error) {
    res.status(500).json({ success: false, error: error.message })
  }
})

app.listen(port, () => {
  console.log(`Servidor corriendo en http://localhost:${port}`)
})
