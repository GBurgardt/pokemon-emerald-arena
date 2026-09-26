import Gameplay from './Gameplay';

const release = 'https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.1/Emerald-Arena-0.10.1.zip';
const repository = 'https://github.com/GBurgardt/pokemon-emerald-arena';

export default function Arena() {
  return (
    <main>
      <header className="masthead">
        <h1>Emerald Arena</h1>
        <nav className="actions" aria-label="Get Emerald Arena">
          <a className="download" href={release}>Download game <span aria-hidden="true">↓</span></a>
          <a className="repo" href={repository}>GitHub repo <span aria-hidden="true">↗</span></a>
        </nav>
      </header>
      <p className="lead">The original Pokémon Emerald, modified for real-time battles.</p>
      <section className="stage" aria-label="Actual game footage">
        <Gameplay />
      </section>
    </main>
  );
}
